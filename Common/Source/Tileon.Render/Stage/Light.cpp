// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Light.hpp"
#include "Tileon.Render/Component.hpp"
#include "Tileon.World/Component/Kinematic/Transform.hpp"
#include "Tileon.World/Component/Spatial/Anchor.hpp"
#include "Tileon.World/Component/Spatial/Extent.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Stage
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Light::Light(Ref<Service::Host> Host)
        : Locator { Host }
    {
        OnRegister(* Host.GetService<Scene::Service>());
        OnLoad(* Host.GetService<Content::Service>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Light::Run(Ref<Graphic::Encoder> Encoder, ConstRef<Director> Director, Graphic::Object Normal)
    {
        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();

        // Calculate the origin of the light stage based on the director's position.
        const IntVector2 Origin(
            Director.GetPosition().GetBaseX(),
            Director.GetPosition().GetBaseY());

        // Reset the light data for the current frame.
        mGlowlightData.clear();
        mSpotlightData.clear();

        // Apply the environment settings for the current frame.
        mQrDrawSkylight.Run<const Skylight>([&](ConstRef<Skylight> Environment)
        {
            auto [Data, Effect] =  Graphics.AllocateTransientBuffer<GpuSkylightLayout>(Graphic::Usage::Uniform, 1);
            Data->SunColor    = Math::Color::FromColor8(Environment.GetSunTint())
                .WithIntensity(Environment.GetBrightness(), Environment.GetSunDirection().GetX());
            Data->SkyColor    = Math::Color::FromColor8(Environment.GetSkyTint())
                .WithIntensity(Environment.GetBrightness(), Environment.GetSunDirection().GetY());
            Data->GroundColor = Math::Color::FromColor8(Environment.GetGroundTint())
                .WithIntensity(Environment.GetBrightness(), 0.0f);

            Encoder.SetPipeline(mPipelines[Enum::Cast(Technique::Skylight)]->GetID());
            Encoder.SetTexture(0, Normal, Graphic::Sampler());
            Encoder.SetUniform(1, Effect);
            Encoder.Draw(3, 0, 0);
            Encoder.ResetBindings();
        });

        // Accumulate the glowlights in the scene and render them in a single batch.
        // TODO: Frustum Culling
        mQrDrawGlowlights.Run<const Transform, const Glowlight, ConstPtr<IntColor8>>([&](
            ConstRef<Transform> Transform,
            ConstRef<Glowlight> Light,
            ConstPtr<IntColor8> Tint)
        {
            const Vector2 Center = Transform.GetWorldspace().GetTranslation() + Vector2(Transform.GetOrigin() - Origin);
            const Color   Color  = (Tint ? Math::Color::FromColor8(* Tint) : Color::White()) * Light.GetIntensity();
            const Vector2 Scale  = Transform.GetWorldspace().GetScale();

            const Real32 Radius = Light.GetRadius() * Max(Scale.GetX(), Scale.GetY());
            mGlowlightData.emplace_back(Center, Radius, Light.GetFalloff(), Color);
        });

        // Accumulate the spotlights in the scene and render them in a single batch.
        // TODO: Frustum Culling
        mQrDrawSpotlights.Run<const Transform, const Spotlight, ConstPtr<IntColor8>>([&](
            ConstRef<Transform> Transform,
            ConstRef<Spotlight> Light,
            ConstPtr<IntColor8> Tint)
        {
            const Vector2 Center = Transform.GetWorldspace().GetTranslation() + Vector2(Transform.GetOrigin() - Origin);
            const Color   Color  = (Tint ? Math::Color::FromColor8(* Tint) : Color::White()) * Light.GetIntensity();

            const Vector2 BasisX    = Transform.GetWorldspace().GetBasisX();
            const Vector2 Direction = Vector2::Normalize(BasisX);
            const Vector2 Angles    = Vector2(Angle::Cosine(Light.GetInnerAngle()), Angle::Cosine(Light.GetOuterAngle()));

            const Real32 Range = Light.GetRange() * BasisX.GetLength();
            mSpotlightData.emplace_back(Center, Range, Light.GetFalloff(), Direction, Angles, Color);
        });

        // Set the projection matrix for the light stage based on the director's position and viewport size.
        const Graphic::Stream Projection = Graphics.AllocateTransientBuffer(
            Graphic::Usage::Uniform, Spanify(Director.GetProjection()));
        Encoder.SetUniform(0, Projection);

        // Render the accumulated glowlights in batches to minimize draw calls and state changes.
        if (const ConstSpan<GpuGlowlightLayout> Data = mGlowlightData; !Data.empty())
        {
            Encoder.SetPipeline(mPipelines[Enum::Cast(Technique::Glowlight)]->GetID());
            Encoder.SetTexture(0, Normal, Graphic::Sampler());
            Encoder.SetVertices(0, Graphics.AllocateTransientBuffer(Graphic::Usage::Vertex, Data));
            Encoder.Draw(4, 0, 0, Data.size());
            Encoder.ResetBindings();
        }

        // Render the accumulated spotlights in batches to minimize draw calls and state changes.
        if (const ConstSpan<GpuSpotlightLayout> Data = mSpotlightData; !Data.empty())
        {
            Encoder.SetPipeline(mPipelines[Enum::Cast(Technique::Spotlight)]->GetID());
            Encoder.SetTexture(0, Normal, Graphic::Sampler());
            Encoder.SetVertices(0, Graphics.AllocateTransientBuffer(Graphic::Usage::Vertex, Data));
            Encoder.Draw(3, 0, 0, Data.size());
            Encoder.ResetBindings();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Light::OnRegister(Ref<Scene::Service> Scene)
    {
        Scene.GetComponent<Glowlight>("Glowlight").AddTrait(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Skylight>("Skylight").AddTrait(Scene::Trait::Serializable, Scene::Trait::Singleton);
        Scene.GetComponent<Spotlight>("Spotlight").AddTrait(Scene::Trait::Serializable, Scene::Trait::Inheritable);

        // Observes changes to the light radial component and updates the corresponding spatial properties of the actor.
        Scene.CreateObserver<Scene::DSL::In<const Glowlight>>(
            "Render::Light::ObsUpdateGlowlightBoundaries",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Glowlight> Light)
            {
                const Real32 Radius = Light.GetRadius();
                Actor.Set(Extent { Vector2(-Radius, -Radius), Vector2(Radius * 2.0f, Radius * 2.0f) });
                Actor.Set(Anchor { Vector2(0.0f, 0.0f) });
            });

        // Observes changes to the light cone component and updates the corresponding spatial properties of the actor.
        Scene.CreateObserver<Scene::DSL::In<const Transform, const Spotlight>>(
            "Render::Light::ObsUpdateSpotlightBoundaries",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Transform> Transform, ConstRef<Spotlight> Light)
            {
                const Angle  Theta = Vector2::Normalize(Transform.GetWorldspace().GetBasisX()).GetAngle();
                const Real32 Range = Light.GetRange();

                const Angle A1 = Theta - Light.GetOuterAngle();
                const Angle A2 = Theta + Light.GetOuterAngle();

                const Vector2 P1(Range * Angle::Cosine(A1), Range * Angle::Sine(A1));
                const Vector2 P2(Range * Angle::Cosine(A2), Range * Angle::Sine(A2));

                Real32 MinX = Math::Min(0.0f, Math::Min(P1.GetX(), P2.GetX()));
                Real32 MaxX = Math::Max(0.0f, Math::Max(P1.GetX(), P2.GetX()));
                Real32 MinY = Math::Min(0.0f, Math::Min(P1.GetY(), P2.GetY()));
                Real32 MaxY = Math::Max(0.0f, Math::Max(P1.GetY(), P2.GetY()));

                constexpr Real32 kHalfPi = Math::kPI<Real32> * 0.5f;

                const SInt32 Start = static_cast<SInt32>(Math::Floor(A1.GetRadians() / kHalfPi));
                const SInt32 End   = static_cast<SInt32>(Math::Floor(A2.GetRadians() / kHalfPi));

                for (SInt32 Side = Start; Side <= End; ++Side)
                {
                    const Angle Cardinal = Side * kHalfPi;

                    if (Cardinal >= A1 && Cardinal <= A2)
                    {
                        const Real32 Cx = Range * Angle::Cosine(Cardinal);
                        const Real32 Cy = Range * Angle::Sine(Cardinal);
                        MinX = Math::Min(MinX, Cx);
                        MaxX = Math::Max(MaxX, Cx);
                        MinY = Math::Min(MinY, Cy);
                        MaxY = Math::Max(MaxY, Cy);
                    }
                }

                Actor.Set(Extent { Vector2(MinX, MinY), Vector2(MaxX - MinX, MaxY - MinY) });
                Actor.Set(Anchor { Vector2(0.0f, 0.0f) });
            });

        // Creates the queries for the light stage.
        mQrDrawGlowlights = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Glowlight, ConstPtr<IntColor8>>
        >("Render::Light::DrawGlowlights", Scene::Cache::Auto);

        mQrDrawSpotlights = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Spotlight, ConstPtr<IntColor8>>
        >("Render::Light::DrawSpotlights", Scene::Cache::Auto);

        mQrDrawSkylight = Scene.CreateQuery<
            Scene::DSL::In<const Skylight>
        >("Render::Light::DrawSkylight", Scene::Cache::Auto);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Light::OnLoad(Ref<Content::Service> Content)
    {
        for (const Technique Type : Enum::Values<Technique>())
        {
            const ConstStr8 Path = Format("Resources://Pipeline/Light/{}.effect", Enum::Name(Type));

            mPipelines[Enum::Cast(Type)] = Content.Load<Graphic::Pipeline>(Path);
        }
    }
}