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

#include "Lightning.hpp"
#include "Tileon.Render/Component.hpp"
#include "Tileon.World/Component.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Pipeline
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Lightning::Lightning(Ref<Engine::Subsystem::Host> Host, ConstRef<Render::Target> Normal, ConstRef<Render::Target> Depth)
        : Locator { Host },
          mNormal { AddressOf(Normal) },
          mDepth  { AddressOf(Depth) }
    {
        OnRegister(* Host.GetService<Scene::Service>());
        OnLoad(* Host.GetService<Content::Service>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Lightning::Run(Ref<Render::Encoder> Encoder)
    {
        ZY_PROFILE_SCOPE("Pipeline::Lightning::Run");

        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();

        // Calculate the origin of the light stage based on the director's position.
        const IntVector3 Origin = IntVector3::FromXZ(IntVector2(
            mDirector->GetPosition().GetBaseX(),
            mDirector->GetPosition().GetBaseY()));

        const Array Textures(mNormal->GetTexture(), mDepth->GetTexture());

        // Reset the light data for the current frame.
        mGlowlightData.Clear();
        mSpotlightData.Clear();

        // Apply the environment settings for the current frame.
        mQrDrawSkylight.Run([&](ConstRef<Skylight> Environment)
        {
            // Premultiplies a tint by the brightness, leaving the alpha channel free to carry custom data.
            const auto Premultiply = [Brightness = Environment.GetBrightness()](IntColor8 Tint, Real32 Alpha)
            {
                const Color Value = Math::Color::FromColor8(Tint);
                return Color(Value.GetRed() * Brightness, Value.GetGreen() * Brightness, Value.GetBlue() * Brightness, Alpha);
            };

            Graphic::Transient<GpuSkylightLayout> Data =  Graphics.AllocateInFlightUniforms<GpuSkylightLayout>(1);
            Data[0].SunColor    = Premultiply(Environment.GetSunTint(),    Environment.GetSunDirection().GetX());
            Data[0].SkyColor    = Premultiply(Environment.GetSkyTint(),    Environment.GetSunDirection().GetY());
            Data[0].GroundColor = Premultiply(Environment.GetGroundTint(), Environment.GetSunDirection().GetZ());
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
        mQrDrawGlowlights.Run([&](
            ConstRef<Transform> Transform,
            ConstRef<Enclosure> Enclosure,
            ConstRef<Glowlight> Light,
            ConstPtr<IntColor8> Tint)
        {
            if (const IntBox AABB = Enclosure.GetVolume(); !AABB.IsAlmostZero() && !mDirector->IsVisible(AABB))
            {
                return;
            }

            const Vector3 Center = Transform.GetWorldspace().GetTranslation() + Vector3(Transform.GetOrigin() - Origin);
            const Color   Tinted = (Tint ? Math::Color::FromColor8(* Tint) : Color::White()) * Light.GetIntensity();
            const Vector3 Scale  = Transform.GetWorldspace().GetScale();

            const Real32 Radius = Light.GetRadius() * Max(Scale.GetX(), Max(Scale.GetY(), Scale.GetZ()));
            const Color  Packed(Tinted.GetRed(), Tinted.GetGreen(), Tinted.GetBlue(), Light.GetFalloff());

            mGlowlightData.Append(Center, Radius, Packed);
        });

        // Accumulate the spotlights in the scene and render them in a single batch.
        mQrDrawSpotlights.Run([&](
            ConstRef<Transform> Transform,
            ConstRef<Enclosure> Enclosure,
            ConstRef<Spotlight> Light,
            ConstPtr<IntColor8> Tint)
        {
            if (const IntBox AABB = Enclosure.GetVolume(); !AABB.IsAlmostZero() && !mDirector->IsVisible(AABB))
            {
                return;
            }

            const Vector3 Center = Transform.GetWorldspace().GetTranslation() + Vector3(Transform.GetOrigin() - Origin);
            const Color   Tinted = (Tint ? Math::Color::FromColor8(* Tint) : Color::White()) * Light.GetIntensity();

            const Vector3 BasisX = Transform.GetWorldspace().GetBasisX();
            const Real32  Length = BasisX.GetLength();

            // A basis with no length has no direction to aim along.
            if (Length < 0.0001f)
            {
                return;
            }

            // The cone fades from the outer angle inwards, so the outer cosine has to stay the smaller of the two.
            const Real32 Inner = Angle::Cosine(Light.GetInnerAngle());
            const Real32 Outer = Min(Angle::Cosine(Light.GetOuterAngle()), Inner - 0.0001f);
            const Color  Packed(Tinted.GetRed(), Tinted.GetGreen(), Tinted.GetBlue(), Light.GetFalloff());

            mSpotlightData.Append(Center, Light.GetRange() * Length, BasisX / Length, Inner, Packed, Outer);
        });

        if (!mGlowlightData.IsEmpty() || !mSpotlightData.IsEmpty())
        {
            Graphic::Transient<GpuPassLayout> Data = Graphics.AllocateInFlightUniforms<GpuPassLayout>(1);
            Data[0].Inverse = mDirector->GetViewProjectionInverse();

            Encoder.SetPass(Data.GetStream());
        }

        // Render the accumulated glowlights in batches to minimize draw calls and state changes.
        if (const ConstSpan<GpuGlowlightLayout> Data = mGlowlightData; !Data.IsEmpty())
        {
            Graphic::Transient<GpuGlowlightLayout> Instances
                = Graphics.AllocateInFlightVertices<GpuGlowlightLayout>(Data.GetSize());
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
                = Graphics.AllocateInFlightVertices<GpuSpotlightLayout>(Data.GetSize());
            Instances.Copy(Data);

            // The cone is bounded by the rectangle its own sphere covers, so it emits a quad like the rest.
            const Graphic::Invocation Invocation = {
                .Count     = 4,
                .Base      = 0,
                .Offset    = 0,
                .Instances = static_cast<UInt32>(Data.GetSize())
            };
            Encoder.Draw(* mTechniques[Enum::Cast(Kind::Spotlight)], Textures, Instances.GetStream(), Invocation);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Lightning::OnRegister(Ref<Scene::Service> Scene)
    {
        Scene.Register(
            Scene::DSL::Declare<Glowlight, Spotlight>(Scene::DSL::Authored),
            Scene::DSL::Declare<Skylight>(Scene::DSL::Serializable, Scene::DSL::Singleton));

        // Observes changes to the light radial component and updates the corresponding spatial properties of the actor.
        Scene.CreateObserver<>(
            "Render::Lightning::ObsUpdateGlowlightBoundaries",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Glowlight> Light)
            {
                const Real32 Radius = Light.GetRadius();
                Actor.Set(Extent(Vector3(-Radius), Vector3(Radius * 2.0f)));
            }, Scene::DSL::Opt(EcsPrefab));

        // Observes changes to the light cone component and updates the corresponding spatial properties of the actor.
        Scene.CreateObserver<>(
            "Render::Lightning::ObsUpdateSpotlightBoundaries",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Spotlight> Light)
            {
                const Real32 Range = Light.GetRange();

                const Angle   A1 = Angle() - Light.GetOuterAngle();
                const Angle   A2 = Light.GetOuterAngle();
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

                const Real32 Spread = Range * Angle::Sine(Light.GetOuterAngle());
                Actor.Set(Extent(Vector3(MinX, -Spread, MinY), Vector3(MaxX - MinX, Spread * 2.0f, MaxY - MinY)));
            }, Scene::DSL::Opt(EcsPrefab));

        // Creates the queries for the light stage.
        mQrDrawGlowlights = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Enclosure, const Glowlight, ConstPtr<IntColor8>>
        >("Render::Lightning::DrawGlowlights", Scene::Cache::Auto);

        mQrDrawSpotlights = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Enclosure, const Spotlight, ConstPtr<IntColor8>>
        >("Render::Lightning::DrawSpotlights", Scene::Cache::Auto);

        mQrDrawSkylight = Scene.CreateQuery<
            Scene::DSL::In<const Skylight>
        >("Render::Lightning::DrawSkylight", Scene::Cache::Auto);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Lightning::OnLoad(Ref<Content::Service> Content)
    {
        for (const Kind Type : Enum::GetValues<Kind>())
        {
            Str Path = Str::Print<"Resources://Technique/Lightning/{0}.vfx">(Enum::GetName(Type));

            mTechniques[Enum::Cast(Type)] = Content.Load<Graphic::Technique>(Move(Path));
        }
    }
}