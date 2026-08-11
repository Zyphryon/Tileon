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

#include "Preview.hpp"
#include "Tileon.Render/Component.hpp"
#include "Tileon.World/Component.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Pipeline
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Preview::Preview(
        Ref<Engine::Subsystem::Host> Host,
        ConstRef<Render::Target>     Albedo,
        ConstRef<Render::Target>     Normal,
        ConstRef<Render::Target>     Depth,
        ConstRef<Render::Target>     Radiance)
        : Locator     { Host },
          mSources    { AddressOf(Albedo), AddressOf(Normal), AddressOf(Depth), AddressOf(Radiance) },
          mDirector   { nullptr },
          mSource     { Kind::Albedo },
          mProperties { 0 }
    {
        OnRegister(* Host.GetService<Scene::Service>());
        OnLoad(* Host.GetService<Content::Service>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Preview::Run(Ref<Render::Encoder> Encoder)
    {
        ZY_PROFILE_SCOPE("Pipeline::Preview::Run");

        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();

        Graphic::Transient<GpuPreviewLayout> Data = Graphics.AllocateInFlightUniforms<GpuPreviewLayout>(1);
        Data[0].Inverse = mDirector->GetViewProjectionInverse();
        Encoder.SetPass(Data.GetStream());

        const Array                   Textures(mSources[Enum::Cast(mSource)]->GetTexture());
        constexpr Graphic::Invocation Invocation = {
            .Count     = 3,
            .Base      = 0,
            .Offset    = 0,
            .Instances = 1
        };
        Encoder.Draw(* mTechniques[Enum::Cast(mSource)], Textures, Invocation);

        if (mSource == Kind::Albedo && HasProperty(Property::Grid))
        {
            DrawGrid(Encoder);
        }

        if ((mSource == Kind::Albedo || mSource == Kind::Radiance) && HasProperty(Property::Boundaries))
        {
            DrawBoundaries(Encoder);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Preview::DrawGrid(Ref<Render::Encoder> Encoder)
    {
        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();

        Graphic::Transient<GpuGridLayout> Data = Graphics.AllocateInFlightUniforms<GpuGridLayout>(1);
        Data[0].Camera    = mDirector->GetViewProjectionInverse();
        Data[0].Dimension = Vector2(Region::kTilesPerX, Region::kTilesPerY);
        Encoder.SetPass(Data.GetStream());

        constexpr Graphic::Invocation Invocation = {
            .Count     = 3,
            .Base      = 0,
            .Offset    = 0,
            .Instances = 1
        };
        Encoder.Draw(* mOverlays[Enum::Cast(Overlay::Grid)], ConstSpan<Graphic::Object>(), Invocation);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Preview::DrawBoundaries(Ref<Render::Encoder> Encoder)
    {
        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();

        // A bound is absolute while the camera is rebased onto its region, so shift each one into the camera's space.
        const IntVector3 Origin = IntVector3::FromXZ(IntVector2(
            mDirector->GetPosition().GetBaseX(),
            mDirector->GetPosition().GetBaseY()));

        // Reset the boundary data for the current frame.
        mBoundaries.Clear();

        // Accumulate the boundaries of the visible entities and render them in a single batch.
        if (mSource == Kind::Albedo)
        {
            mQrDrawGeometryBoundaries.Run([&](Scene::Entity Actor, ConstRef<Enclosure> Enclosure)
            {
                if (const IntBox Volume = Enclosure.GetBox(); mDirector->IsVisible(Volume))
                {
                    const IntBox  Local = Volume - Origin;
                    const Vector3 Lower(Local.GetMinimum());
                    const Vector3 Upper(Local.GetMaximum());

                    mBoundaries.Append((Lower + Upper) * 0.5f, (Upper - Lower) * 0.5f);
                }
            });
        }
        else
        {
            mQrDrawLightBoundaries.Run([&](Scene::Entity Actor, ConstRef<Enclosure> Enclosure)
            {
                if (const IntBox Volume = Enclosure.GetBox(); mDirector->IsVisible(Volume))
                {
                    const IntBox Local = Volume - Origin;
                    const Vector3 Lower(Local.GetMinimum());
                    const Vector3 Upper(Local.GetMaximum());

                    mBoundaries.Append((Lower + Upper) * 0.5f, (Upper - Lower) * 0.5f);
                }
            });
        }

        if (const ConstSpan<GpuBoundaryLayout> Data = mBoundaries; !Data.IsEmpty())
        {
            Graphic::Transient<GpuBoundaryLayout> Instances
                = Graphics.AllocateInFlightVertices<GpuBoundaryLayout>(Data.GetSize());
            Instances.Copy(Data);

            const Bool    Aligned   = mDirector->GetProjection().IsAxisAligned();
            const Overlay Technique = Aligned ? Overlay::Boundary_Flat : Overlay::Boundary;

            const Graphic::Invocation Invocation = {
                .Count     = Aligned ? 8u : 24u,
                .Base      = 0,
                .Offset    = 0,
                .Instances = static_cast<UInt32>(Data.GetSize())
            };
            Encoder.Draw(* mOverlays[Enum::Cast(Technique)], { }, Instances.GetStream(), Invocation);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Preview::OnRegister(Ref<Scene::Service> Scene)
    {
        mQrDrawGeometryBoundaries = Scene.CreateQuery<
            Scene::DSL::In<const Enclosure>,
            Scene::DSL::Not<Glowlight, Spotlight>
        >("Render::Preview::DrawGeometryBoundaries", Scene::Cache::Auto);

        mQrDrawLightBoundaries = Scene.CreateQuery<
            Scene::DSL::In<const Enclosure>,
            Scene::DSL::Not<Sprite, Text>
        >("Render::Preview::DrawLightBoundaries", Scene::Cache::Auto);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Preview::OnLoad(Ref<Content::Service> Content)
    {
        for (const Kind Type : Enum::GetValues<Kind>())
        {
            Str Path = Str::Print<"Resources://Technique/Preview/Preview_{0}.vfx">(Enum::GetName(Type));

            mTechniques[Enum::Cast(Type)] = Content.Load<Graphic::Technique>(Move(Path));
        }

        for (const Overlay Type : Enum::GetValues<Overlay>())
        {
            Str Path = Str::Print<"Resources://Technique/Preview/{0}.vfx">(Enum::GetName(Type));

            mOverlays[Enum::Cast(Type)] = Content.Load<Graphic::Technique>(Move(Path));
        }
    }
}