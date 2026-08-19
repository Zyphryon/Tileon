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
        ZY_PROFILE_SCOPE("Stage::Preview::Run");

        // Color is what the technique reads by default, so only a buffer that has to be decoded names a variant.
        const Bool Decoded = (mSource == Kind::Normal || mSource == Kind::Depth);
        Encoder.Begin(* mTechnique)
               .SetImage("Source"_Hash, mSources[Enum::Cast(mSource)]->GetTexture())
               .SetVariant(Decoded ? mTechnique->ResolveByName(Enum::GetName(mSource)) : 0)
               .DrawFullscreen();

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
        Encoder.SetPass(GpuGridLayout {
            .Dimension = Vector2(Region::kUnitsPerX, Region::kUnitsPerY)
        });
        Encoder.Begin(* mOverlays[Enum::Cast(Overlay::Grid)]).DrawFullscreen();
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
            ConstRetainer<Graphic::Technique> Technique = mOverlays[Enum::Cast(Overlay::Boundary)];
            Encoder.Begin(* Technique)
                   .SetVariant(Flat ? Technique->ResolveByName("Flat") : 0)
                   .Draw(Graphics.AllocateInFlightVertices<GpuBoundaryLayout>(Data), Invocation);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Preview::OnRegister(Ref<Scene::Service> Scene)
    {
        mQrDrawGeometryBoundaries = Scene.CreateQuery<
            Scene::DSL::In<const Enclosure>,
            Scene::DSL::Or<Sprite, Lettering>
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
        mTechnique = Content.Load<Graphic::Technique>("Resources://Technique/Preview/Preview.vfx");

        for (const Overlay Type : Enum::GetValues<Overlay>())
        {
            Str Path = Str::Print<"Resources://Technique/Preview/{0}.vfx">(Enum::GetName(Type));

            mOverlays[Enum::Cast(Type)] = Content.Load<Graphic::Technique>(Move(Path));
        }
    }
}