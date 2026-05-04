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
#include "Tileon.Visual/Component.hpp"
#include "Tileon.World/Component/Extent.hpp"
#include "Tileon.World/Component/Spatial.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Visual::Stage
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

        const IntVector2 Origin(Director.GetPosition().GetBaseX(), Director.GetPosition().GetBaseY());

        // Accumulate glow lights into the instance buffer.
        // TODO: Frustum Culling
        mGlowlightData.clear();

        mQrDrawGlowlights.Run<const Sector, const Worldspace, const Glowlight, ConstPtr<Tint>>([&](
            Sector               Sector,
            ConstRef<Worldspace> Worldspace,
            ConstRef<Glowlight>  Light,
            ConstPtr<Tint>       Tint)
        {
            const Vector2 Offset(Sector - Origin);

            const Color Color = Tint ? Math::Color::FromColor8(* Tint) : Color::White();

            // TODO: Prettier
            mGlowlightData.emplace_back(
                Worldspace.GetTranslation() + Offset,
                Light.GetRadius(),
                Light.GetFalloff(),
                Color * Light.GetIntensity());
        });

        // Accumulate spot lights into the instance buffer.
        // TODO: Frustum Culling
        mSpotlightData.clear();

        mQrDrawSpotlights.Run<const Sector, const Worldspace, const Spotlight, ConstPtr<Tint>>([&](
            Sector               Sector,
            ConstRef<Worldspace> Worldspace,
            ConstRef<Spotlight>  Light,
            ConstPtr<Tint>       Tint)
        {
            const Vector2 Offset(Sector - Origin);

            const Color Color = Tint ? Math::Color::FromColor8(* Tint) : Color::White();

            // TODO: Prettier
            mSpotlightData.emplace_back(
                Worldspace.GetTranslation() + Offset,
                Light.GetRange(),
                Light.GetFalloff(),
                Vector2::Normalize(Worldspace.GetBasisX()),
                Vector2(Angle::Cosine(Light.GetInnerAngle()), Angle::Cosine(Light.GetOuterAngle())),
                Color * Light.GetIntensity());
        });

        // Draw the glow lights using the accumulated instance data.
        if (!mGlowlightData.empty())
        {
            const Graphic::Stream Instances
                = Graphics.AllocateTransientBuffer<GlowlightLayout>(Graphic::Usage::Vertex, mGlowlightData);

            Encoder.SetPipeline(mPipelines[Enum::Cast(Technique::Glowlight)]->GetID());
            Encoder.SetTexture(0, Normal, Graphic::Sampler());
            Encoder.SetVertices(0, Instances);
            Encoder.Draw(4, 0, 0, mGlowlightData.size());
            Encoder.ResetBindings();
        }

        // Draw the spot lights using the accumulated instance data.
        if (!mSpotlightData.empty())
        {
            const Graphic::Stream Instances
                = Graphics.AllocateTransientBuffer<SpotlightLayout>(Graphic::Usage::Vertex, mSpotlightData);

            Encoder.SetPipeline(mPipelines[Enum::Cast(Technique::Spotlight)]->GetID());
            Encoder.SetTexture(0, Normal, Graphic::Sampler());
            Encoder.SetVertices(0, Instances);
            Encoder.Draw(4, 0, 0, mSpotlightData.size());
            Encoder.ResetBindings();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Light::OnRegister(Ref<Scene::Service> Scene)
    {
        Scene.GetComponent<Spotlight>("Spotlight").AddTrait(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Glowlight>("Glowlight").AddTrait(Scene::Trait::Serializable, Scene::Trait::Inheritable);

        // Observes changes to the light radial component and updates the corresponding spatial properties of the actor.
        Scene.CreateObserver<Scene::DSL::In<const Glowlight>>(
            "Visual::Light::ObsUpdateGlowlightBoundaries",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Glowlight> Light)
            {
                const Real32 Radius = Light.GetRadius();
                Actor.Set(Extent { Vector2(-Radius, -Radius), Vector2(Radius * 2.0f, Radius * 2.0f) });
                Actor.Set(Origin { Vector2(0.0f, 0.0f) });
            });

        // Observes changes to the light cone component and updates the corresponding spatial properties of the actor.
        Scene.CreateObserver<Scene::DSL::In<const Worldspace, const Spotlight>>(
            "Visual::Light::ObsUpdateSpotlightBoundaries",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Worldspace> Worldspace, ConstRef<Spotlight> Light)
            {
                const Angle  Theta = Vector2::Normalize(Worldspace.GetBasisX()).GetAngle();
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
                Actor.Set(Origin { Vector2(0.0f, 0.0f) });
            });

        // Creates the queries for the light stage.
        mQrDrawGlowlights = Scene.CreateQuery<
            Scene::DSL::Up<const Sector>,           // TODO: Remove Sector
            Scene::DSL::In<const Worldspace, const Glowlight, ConstPtr<Tint>>
        >("Visual::Light::DrawGlowlights", Scene::Cache::Auto);

        mQrDrawSpotlights = Scene.CreateQuery<
            Scene::DSL::Up<const Sector>,           // TODO: Remove Sector
            Scene::DSL::In<const Worldspace, const Spotlight, ConstPtr<Tint>>
        >("Visual::Light::DrawSpotlights", Scene::Cache::Auto);
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