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

        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();

        // The scene authors its exposure alongside the rest of the environment, so the stage follows it.
        Real32 Exposure = 1.0f;

        if (const ConstPtr<Skylight> Environment = mWorld.TryGet<const Skylight>())
        {
            Exposure = Environment->GetExposure();
        }

        // The tone map's knee sits at a fixed place in the range, so exposure is what moves the scene into it.
        Graphic::Transient<GpuCompositeLayout> Data = Graphics.AllocateInFlightUniforms<GpuCompositeLayout>(1);
        Data[0].Exposure = Vector4(Exposure, 0.0f, 0.0f, 0.0f);
        Encoder.SetPass(Data.GetStream());

        const Array                   Textures(mAlbedo->GetTexture(), mRadiance->GetTexture());
        constexpr Graphic::Invocation Invocation = {
            .Count     = 3,
            .Base      = 0,
            .Offset    = 0,
            .Instances = 1
        };
        Encoder.Draw(* mTechniques[Enum::Cast(Kind::Composite)], Textures, Invocation);
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