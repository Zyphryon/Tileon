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

#include "Renderer.hpp"
#include "Stage/Composite.hpp"
#include "Stage/Preview.hpp"
#include "Stage/Geometry.hpp"
#include "Stage/Lighting.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Renderer::Renderer(Ref<Engine::Subsystem::Host> Host, Bool Immediate, Real32 Density)
        : Locator    { Host },
          mSplatset  { Host },
          mRenderer  { Host },
          mPreview   { nullptr },
          mOutput    { Target::Final },
          mImmediate { Immediate }
    {
        OnCreate(Host, Immediate, Density);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::Load()
    {
        mSplatset.Load();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::Save()
    {
        mSplatset.Save();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::Resize(UInt16 Width, UInt16 Height)
    {
        mRenderer.Resize(Width, Height);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::Present(ConstRef<Director> Director)
    {
        // Hand the frame's director to the stages that resolve their draws from it.
        mRenderer.GetPass<Stage::Geometry>(Enum::Cast(Phase::Geometry)).SetDirector(Director);
        mRenderer.GetPass<Stage::Lighting>(Enum::Cast(Phase::Light)).SetDirector(Director);

        if (mPreview)
        {
            mPreview->SetDirector(Director);
        }

        // Build the frame-global uniform the renderer binds for every stage.
        const GpuFrameLayout Global {
            .Camera        = Director.GetViewProjection(),
            .CameraInverse = Director.GetViewProjectionInverse()
        };
        mRenderer.Run(GetService<Graphic::Service>().AllocateInFlightUniforms<GpuFrameLayout>(Global));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::SetOutput(Target Type)
    {
        Phase Producer;

        // Find the phase that writes the requested target, and how the preview phase has to read it.
        switch (Type)
        {
        case Target::Albedo:
            Producer = Phase::Geometry;
            break;
        case Target::Normal:
            Producer = Phase::Geometry;
            break;
        case Target::Depth:
            Producer = Phase::Geometry;
            break;
        case Target::Radiance:
            Producer = Phase::Light;
            break;
        default:
            Producer = Phase::Composite;
            break;
        }

        // Everything past the producer is skipped, except the preview phase, which resolves what it wrote.
        for (const Phase Stage : Enum::GetValues<Phase>())
        {
            if (Stage != Phase::Preview || mPreview)
            {
                mRenderer.GetPass(Enum::Cast(Stage)).SetActive(Enum::Cast(Stage) <= Enum::Cast(Producer));
            }
        }

        // The composed image is already what the view wants, so only a raw buffer needs resolving.
        if (mPreview)
        {
            mPreview->SetActive(Type != Target::Final);

            if (Type != Target::Final)
            {
                mPreview->SetSource(static_cast<Stage::Preview::Source>(Enum::Cast(Type)));
            }
        }

        mOutput = Type;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::OnCreate(Ref<Engine::Subsystem::Host> Host, Bool Immediate, Real32 Density)
    {
        // Declare the managed targets in the same order as Target, with the preview image last.
        Ref<Render::Target> Albedo   = mRenderer.AddTarget({ .Format = Graphic::TextureFormat::RGBA8UIntNorm_sRGB });
        Ref<Render::Target> Normal   = mRenderer.AddTarget({ .Format = Graphic::TextureFormat::RGBA8UIntNorm      });
        Ref<Render::Target> Depth    = mRenderer.AddTarget({ .Format = Graphic::TextureFormat::D24S8UIntNorm      });
        Ref<Render::Target> Radiance = mRenderer.AddTarget({ .Format = Graphic::TextureFormat::R11G11B10Float     });

        // Geometry: rasterize the scene into the albedo and normal buffers, depth-tested.
        Ref<Stage::Geometry> Geometry = mRenderer.AddPass<Stage::Geometry>(Host, mSplatset, Density);
        Geometry.AddColor({ .Target = AddressOf(Albedo), .Tint = Color::Black() });
        Geometry.AddColor({ .Target = AddressOf(Normal), .Tint = Color(0.5f, 1.0f, 0.5f, 1.0f) });
        Geometry.SetDepth({ .Target = AddressOf(Depth) });

        // Light: accumulates each light's contribution into the radiance buffer, sampling the normal buffer.
        Ref<Stage::Lighting> Light = mRenderer.AddPass<Stage::Lighting>(Host, Normal, Depth);
        Light.AddColor({ .Target = AddressOf(Radiance), .Tint = Color::Black() });

        // Composite: resolves albedo against radiance into the final buffer.
        Ref<Stage::Composite> Composite = mRenderer.AddPass<Stage::Composite>(Host, Albedo, Radiance);

        if (Immediate)
        {
            Composite.AddColor({ .Target = nullptr, .Load = Graphic::Action::Discard });
        }
        else
        {
            Ref<Render::Target> Final = mRenderer.AddTarget({ .Format = Graphic::TextureFormat::RGBA8UIntNorm_sRGB });
            Composite.AddColor({ .Target = AddressOf(Final), .Load = Graphic::Action::Discard });

            // Preview: resolves a raw buffer onto the view in place of the composed scene.
            Ref<Stage::Preview> Preview = mRenderer.AddPass<Stage::Preview>(Host, Albedo, Normal, Depth, Radiance);
            Preview.AddColor({ .Target = AddressOf(Final), .Tint = Color::Black() });

            mPreview = AddressOf(Preview);
        }

        SetOutput(mOutput);
    }
}