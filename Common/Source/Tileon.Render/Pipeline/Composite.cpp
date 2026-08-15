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

#include "Composite.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Pipeline
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Composite::Composite(Ref<Engine::Subsystem::Host> Host, ConstRef<Render::Target> Albedo, ConstRef<Render::Target> Radiance)
        : Locator   { Host },
          mWorld    { Host.GetService<Scene::Service>()->GetWorld() },
          mAlbedo   { AddressOf(Albedo) },
          mRadiance { AddressOf(Radiance) }
    {
        OnLoad(* Host.GetService<Content::Service>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Composite::Run(Ref<Render::Encoder> Encoder)
    {
        ZY_PROFILE_SCOPE("Pipeline::Composite::Run");

        // The scene authors its exposure and its curve alongside the rest of the environment.
        Real32            Exposure = 1.0f;
        Skylight::Tonemap Tonemap  = Skylight::Tonemap::GT7;

        if (const ConstPtr<Skylight> Environment = mWorld.TryGet<const Skylight>())
        {
            Exposure = Environment->GetExposure();
            Tonemap  = Environment->GetTonemap();
        }

        // The tone map's knee sits at a fixed place in the range, so exposure is what moves the scene into it.
        Encoder.SetPass(GpuCompositeLayout {
            .Exposure = Vector4(Exposure, 0.0f, 0.0f, 0.0f)
        });

        ConstRetainer<Graphic::Technique> Technique = mTechniques[Enum::Cast(Kind::Composite)];

        // The base variant carries the Gran Turismo curve, so only another operator names a feature to compile in.
        const Graphic::Technique::Key Variant = Tonemap == Skylight::Tonemap::GT7
            ? 0
            : Technique->ResolveByName(Enum::GetName(Tonemap));

        // Each buffer binds under the name the signature declares it by, rather than by the order it is passed in.
        Encoder.Begin(* Technique)
               .SetImage("Albedo"_Hash,   mAlbedo->GetTexture())
               .SetImage("Radiance"_Hash, mRadiance->GetTexture())
               .SetVariant(Variant)
               .DrawFullscreen();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Composite::OnLoad(Ref<Content::Service> Content)
    {
        for (const Kind Type : Enum::GetValues<Kind>())
        {
            Str Path = Str::Print<"Resources://Technique/Composite/{0}.vfx">(Enum::GetName(Type));

            mTechniques[Enum::Cast(Type)] = Content.Load<Graphic::Technique>(Move(Path));
        }
    }
}