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
#include "Tileon.World/Component.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Stage
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Light::Light(Ref<Engine::Subsystem::Host> Host, ConstRef<Render::Target> Normal)
        : Locator { Host },
          mNormal { AddressOf(Normal) }
    {
        OnRegister(* Host.GetService<Scene::Service>());
        OnLoad(* Host.GetService<Content::Service>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Light::Run(Ref<Render::Encoder> Encoder)
    {
        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();
        ConstRef<Director>    Director = (* mDirector);

        // Calculate the origin of the light stage based on the director's position.
        const IntVector2 Origin(
            Director.GetPosition().GetBaseX(),
            Director.GetPosition().GetBaseY());

        // The normal buffer is the sole input every light technique samples.
        const Array Textures = { mNormal->GetTexture() };

        // Reset the light data for the current frame.
        mGlowlightData.Clear();
        mSpotlightData.Clear();

        // Apply the environment settings for the current frame.
        mQrDrawSkylight.Run<const Skylight>([&](ConstRef<Skylight> Environment)
        {
            const Color SunColor    = Math::Color::FromColor8(Environment.GetSunTint());
            const Color SkyColor    = Math::Color::FromColor8(Environment.GetSkyTint());
            const Color GroundColor = Math::Color::FromColor8(Environment.GetGroundTint());

            Graphic::Transient<GpuSkylightLayout> Data =  Graphics.AllocateTransientUniforms<GpuSkylightLayout>(1);
            Data[0].SunColor    = SunColor.WithIntensity(Environment.GetBrightness(), Environment.GetSunDirection().GetX());
            Data[0].SkyColor    = SkyColor.WithIntensity(Environment.GetBrightness(), Environment.GetSunDirection().GetY());
            Data[0].GroundColor = GroundColor.WithIntensity(Environment.GetBrightness(), 0.0f);
            Encoder.SetPass(Data.GetStream());

            constexpr Graphic::Invocation Invocation = {
                .Count     = 3,
                .Base      = 0,
                .Offset    = 0,
                .Instances = 1
            };
            Encoder.Draw(* mTechniques[Enum::Cast(Kind::Skylight)], Textures, Invocation);
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
            mGlowlightData.Append(Center, Radius, Light.GetFalloff(), Color);
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
            mSpotlightData.Append(Center, Range, Light.GetFalloff(), Direction, Angles, Color);
        });

        // Render the accumulated glowlights in batches to minimize draw calls and state changes.
        if (const ConstSpan<GpuGlowlightLayout> Data = mGlowlightData; !Data.IsEmpty())
        {
            Graphic::Transient<GpuGlowlightLayout> Instances
                = Graphics.AllocateTransientVertices<GpuGlowlightLayout>(Data.GetSize());
            Instances.Copy(Data);

            const Graphic::Invocation Invocation = {
                .Count     = 4,
                .Base      = 0,
                .Offset    = 0,
                .Instances = static_cast<UInt32>(Data.GetSize())
            };
            Encoder.Draw(* mTechniques[Enum::Cast(Kind::Glowlight)], Textures, Instances.GetStream(), Invocation);
        }

        // Render the accumulated spotlights in batches to minimize draw calls and state changes.
        if (const ConstSpan<GpuSpotlightLayout> Data = mSpotlightData; !Data.IsEmpty())
        {
            Graphic::Transient<GpuSpotlightLayout> Instances
                = Graphics.AllocateTransientVertices<GpuSpotlightLayout>(Data.GetSize());
            Instances.Copy(Data);

            const Graphic::Invocation Invocation = {
                .Count     = 3,
                .Base      = 0,
                .Offset    = 0,
                .Instances = static_cast<UInt32>(Data.GetSize())
            };
            Encoder.Draw(* mTechniques[Enum::Cast(Kind::Spotlight)], Textures, Instances.GetStream(), Invocation);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Light::OnRegister(Ref<Scene::Service> Scene)
    {
        Scene.GetComponent<Glowlight>("Glowlight").Grant(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Skylight>("Skylight").Grant(Scene::Trait::Serializable, Scene::Trait::Singleton);
        Scene.GetComponent<Spotlight>("Spotlight").Grant(Scene::Trait::Serializable, Scene::Trait::Inheritable);

        // Observes changes to the light radial component and updates the corresponding spatial properties of the actor.
        Scene.CreateObserver<Scene::DSL::In<const Glowlight>>(
            "Render::Light::ObsUpdateGlowlightBoundaries",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Glowlight> Light)
            {
                const Real32 Radius = Light.GetRadius();
                Actor.Set(Extent { Vector2(-Radius, -Radius), Vector2(Radius * 2.0f, Radius * 2.0f) });
                Actor.Set(Anchor { Vector2(0.0f, 0.0f) });
            }, Scene::DSL::Opt(EcsPrefab));

        // Observes changes to the light cone component and updates the corresponding spatial properties of the actor.
        Scene.CreateObserver<Scene::DSL::In<const Spotlight>>(
            "Render::Light::ObsUpdateSpotlightBoundaries",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Spotlight> Light)
            {
                const Real32 Range = Light.GetRange();

                const Angle A1 = Angle() - Light.GetOuterAngle();
                const Angle A2 = Light.GetOuterAngle();

                const Vector2 P1(Range * Angle::Cosine(A1), Range * Angle::Sine(A1));
                const Vector2 P2(Range * Angle::Cosine(A2), Range * Angle::Sine(A2));

                Real32 MinX = Min(0.0f, P1.GetX(), P2.GetX());
                Real32 MaxX = Max(0.0f, P1.GetX(), P2.GetX());
                Real32 MinY = Min(0.0f, P1.GetY(), P2.GetY());
                Real32 MaxY = Max(0.0f, P1.GetY(), P2.GetY());

                constexpr Real32 kHalfPi = kPI<Real32> * 0.5f;

                const SInt32 Start = static_cast<SInt32>(Floor(A1.GetRadians() / kHalfPi));
                const SInt32 End   = static_cast<SInt32>(Floor(A2.GetRadians() / kHalfPi));

                for (SInt32 Side = Start; Side <= End; ++Side)
                {
                    const Angle Cardinal = Side * kHalfPi;

                    if (Cardinal >= A1 && Cardinal <= A2)
                    {
                        const Real32 Cx = Range * Angle::Cosine(Cardinal);
                        const Real32 Cy = Range * Angle::Sine(Cardinal);
                        MinX = Min(MinX, Cx);
                        MaxX = Max(MaxX, Cx);
                        MinY = Min(MinY, Cy);
                        MaxY = Max(MaxY, Cy);
                    }
                }

                Actor.Set(Extent { Vector2(MinX, MinY), Vector2(MaxX - MinX, MaxY - MinY) });
                Actor.Set(Anchor { Vector2(0.0f, 0.0f) });
            }, Scene::DSL::Opt(EcsPrefab));

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
        for (const Kind Type : Enum::GetValues<Kind>())
        {
            Str Path = Str::Print<"Resources://Technique/Light/{0}.vfx">(Enum::GetName(Type));

            mTechniques[Enum::Cast(Type)] = Content.Load<Graphic::Technique>(Move(Path));
        }
    }
}