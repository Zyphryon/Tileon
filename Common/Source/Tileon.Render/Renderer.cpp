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
#include "Pipeline/Composite.hpp"
#include "Pipeline/Preview.hpp"
#include "Pipeline/Geometry.hpp"
#include "Pipeline/Lightning.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Renderer::Renderer(Ref<Engine::Subsystem::Host> Host, Bool Immediate)
        : Locator    { Host },
          mRenderer  { Host },
          mTileset   { Host },
          mPreview   { nullptr },
          mOutput    { Target::Final },
          mImmediate { Immediate }
    {
        OnCreate(Host, Immediate);
        OnRegister(* Host.GetService<Scene::Service>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::Load()
    {
        mTileset.Load();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::Save()
    {
        mTileset.Save();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::Resize(UInt16 Width, UInt16 Height)
    {
        mRenderer.Resize(Width, Height);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::SetDensity(UInt16 Density)
    {
        mRenderer.GetPass<Pipeline::Geometry>(Enum::Cast(Phase::Geometry)).SetDensity(Density);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::Present(ConstRef<Director> Director)
    {
        // Hand the frame's director to the stages that resolve their draws from it.
        mRenderer.GetPass<Pipeline::Geometry>(Enum::Cast(Phase::Geometry)).SetDirector(Director);
        mRenderer.GetPass<Pipeline::Lightning>(Enum::Cast(Phase::Light)).SetDirector(Director);

        if (mPreview)
        {
            mPreview->SetDirector(Director);
        }

        // Build the frame-global uniform (the director's view-projection) the renderer binds for every stage.
        Graphic::Transient<Matrix4x4> Global = GetService<Graphic::Service>().AllocateInFlightUniforms<Matrix4x4>(1);
        Global[0] = Director.GetViewProjection();

        mRenderer.Run(Global.GetStream());
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
            // Composing into the display declares no preview phase, so there is no pass standing behind it.
            if (Stage == Phase::Preview && !mPreview)
            {
                continue;
            }

            mRenderer.GetPass(Enum::Cast(Stage)).SetActive(Enum::Cast(Stage) <= Enum::Cast(Producer));
        }

        // The composed image is already what the view wants, so only a raw buffer needs resolving.
        if (mPreview)
        {
            mPreview->SetActive(Type != Target::Final);

            if (Type != Target::Final)
            {
                mPreview->SetSource(static_cast<Pipeline::Preview::Kind>(Enum::Cast(Type)));
            }
        }

        mOutput = Type;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::OnCreate(Ref<Engine::Subsystem::Host> Host, Bool Immediate)
    {
        // Declare the managed targets in the same order as Target, with the preview image last.
        Ref<Render::Target> Albedo   = mRenderer.AddTarget({ .Format = Graphic::TextureFormat::RGBA8UIntNorm_sRGB });
        Ref<Render::Target> Normal   = mRenderer.AddTarget({ .Format = Graphic::TextureFormat::RGBA8UIntNorm      });
        Ref<Render::Target> Depth    = mRenderer.AddTarget({ .Format = Graphic::TextureFormat::D24S8UIntNorm      });
        Ref<Render::Target> Radiance = mRenderer.AddTarget({ .Format = Graphic::TextureFormat::R11G11B10Float     });

        // Geometry: rasterize the scene into the albedo and normal buffers, depth-tested.
        Ref<Pipeline::Geometry> Geometry = mRenderer.AddPass<Pipeline::Geometry>(Host, mTileset);
        Geometry.AddColor({ .Target = AddressOf(Albedo), .Tint = Color::Black() });
        Geometry.AddColor({ .Target = AddressOf(Normal), .Tint = Color(0.5f, 1.0f, 0.5f, 1.0f) });
        Geometry.SetDepth({ .Target = AddressOf(Depth) });

        // Light: accumulates each light's contribution into the radiance buffer, sampling the normal buffer.
        Ref<Pipeline::Lightning> Light = mRenderer.AddPass<Pipeline::Lightning>(Host, Normal, Depth);
        Light.AddColor({ .Target = AddressOf(Radiance), .Tint = Color::Black() });

        // Composite: resolves albedo against radiance into the final buffer.
        Ref<Pipeline::Composite> Composite = mRenderer.AddPass<Pipeline::Composite>(Host, Albedo, Radiance);

        if (Immediate)
        {
            Composite.AddColor({ .Target = nullptr, .Load = Graphic::Action::Discard });
        }
        else
        {
            Ref<Render::Target> Final = mRenderer.AddTarget({ .Format = Graphic::TextureFormat::RGBA8UIntNorm_sRGB });
            Composite.AddColor({ .Target = AddressOf(Final), .Load = Graphic::Action::Discard });

            // Preview: resolves a raw buffer onto the view in place of the composed scene.
            Ref<Pipeline::Preview> Preview = mRenderer.AddPass<Pipeline::Preview>(Host, Albedo, Normal, Depth, Radiance);
            Preview.AddColor({ .Target = AddressOf(Final), .Tint = Color::Black() });

            mPreview = AddressOf(Preview);
        }

        SetOutput(mOutput);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::OnRegister(Ref<Scene::Service> Scene)
    {
        // System for ticking the tileset animations based on the absolute time of the scene.
        Scene.CreateSystem<>(
            "Visual::ComputeTilesetAnimation",
            EcsOnUpdate,
            Scene::Execution::Immediate,
            [this](ConstRef<Scene::Clock> Clock)
            {
                mTileset.Tick(Clock.GetAbsolute());
            });
    }
}