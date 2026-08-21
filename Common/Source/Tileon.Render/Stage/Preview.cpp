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

namespace Tileon::Stage
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Preview::Preview(
        Ref<Engine::Subsystem::Host> Host,
        ConstRef<Render::Target>     Albedo,
        ConstRef<Render::Target>     Normal,
        ConstRef<Render::Target>     Depth,
        ConstRef<Render::Target>     Radiance)
        : Locator   { Host },
          mSources  { AddressOf(Albedo), AddressOf(Normal), AddressOf(Depth), AddressOf(Radiance) },
          mDirector { nullptr },
          mSource   { Source::Albedo },
          mOverlays { 0 }
    {
        OnRegister(* Host.GetService<Scene::Service>());
        OnLoad(* Host.GetService<Content::Service>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Preview::Run(Ref<Render::Encoder> Encoder)
    {
        ZY_PROFILE_SCOPE("Stage::Preview::Run");

        DrawPreview(Encoder);

        if (mSource == Source::Albedo && HasOverlay(Overlay::Grid))
        {
            DrawGrid(Encoder);
        }

        if ((mSource == Source::Albedo || mSource == Source::Radiance) && HasOverlay(Overlay::Boundaries))
        {
            DrawBoundaries(Encoder);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Preview::DrawPreview(Ref<Render::Encoder> Encoder)
    {
        Render::Encoder::Binder Binder = Encoder.Begin(* mTechniques[Enum::Cast(Kind::Preview)]);
        Binder.SetImage("Source"_Hash, mSources[Enum::Cast(mSource)]->GetTexture());

        if (mSource == Source::Normal || mSource == Source::Depth)
        {
            Binder.SetVariant(Enum::GetName(mSource));
        }
        Binder.DrawFullscreen();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Preview::DrawGrid(Ref<Render::Encoder> Encoder)
    {
        Encoder.SetPass(GpuGridLayout {
            .Dimension = Vector2(Region::kUnitsPerX, Region::kUnitsPerY)
        });
        Encoder.Begin(* mTechniques[Enum::Cast(Kind::Grid)]).DrawFullscreen();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Preview::DrawBoundaries(Ref<Render::Encoder> Encoder)
    {
        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();

        // A bound is absolute while the camera is rebased onto its region, so shift each one into the camera's space.
        const IntVector3 Origin = mDirector->GetOrigin();

        // Reset the boundary data for the current frame.
        mBoundaries.Clear();

        // Accumulate the boundaries of the visible entities and render them in a single batch.
        if (mSource == Source::Albedo)
        {
            mQrDrawGeometryBoundaries.Run([&](Scene::Entity Actor, ConstRef<Enclosure> Enclosure)
            {
                if (const IntBox Volume = Enclosure.GetVolume(); mDirector->IsVisible(Volume))
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
            mQrDrawLightingBoundaries.Run([&](Scene::Entity Actor, ConstRef<Enclosure> Enclosure)
            {
                if (const IntBox Volume = Enclosure.GetVolume(); mDirector->IsVisible(Volume))
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
            const Bool Flat = mDirector->GetProjection().IsAxisAligned();

            const Graphic::Invocation Invocation = {
                .Count     = Flat ? 8u : 24u,
                .Base      = 0,
                .Offset    = 0,
                .Instances = static_cast<UInt32>(Data.GetSize())
            };

            Render::Encoder::Binder Binder = Encoder.Begin(* mTechniques[Enum::Cast(Kind::Boundary)]);

            if (Flat)
            {
                Binder.SetVariant("Flat");
            }
            Binder.Draw(Graphics.AllocateInFlightVertices<GpuBoundaryLayout>(Data), Invocation);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Preview::OnRegister(Ref<Scene::Service> Scene)
    {
        mQrDrawGeometryBoundaries = Scene.CreateQuery<
            Scene::DSL::In<const Enclosure>,
            Scene::DSL::Or<Sprite, Label>
        >("Render::Preview::DrawGeometryBoundaries", Scene::Cache::Auto);

        mQrDrawLightingBoundaries = Scene.CreateQuery<
            Scene::DSL::In<const Enclosure>,
            Scene::DSL::Or<Glowlight, Spotlight>
        >("Render::Preview::DrawLightingBoundaries", Scene::Cache::Auto);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Preview::OnLoad(Ref<Content::Service> Content)
    {
        for (const Kind Type : Enum::GetValues<Kind>())
        {
            Str Path = Str::Print<"Resources://Technique/Preview/{0}.vfx">(Enum::GetName(Type));

            mTechniques[Enum::Cast(Type)] = Content.Load<Graphic::Technique>(Move(Path));
        }
    }
}