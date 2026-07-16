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
#include "Stage/Geometry.hpp"
#include "Stage/Light.hpp"
#include "Stage/Composite.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Renderer::Renderer(Ref<Engine::Subsystem::Host> Host, Bool Immediate)
        : Locator   { Host },
          mRenderer { Host },
          mTileset  { Host }
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

    void Renderer::Present(ConstRef<Director> Director)
    {
        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();

        // Build the frame-global uniform (the director's view-projection) the renderer binds for every stage.
        Graphic::Transient<Matrix4x4> Global = Graphics.AllocateTransientUniforms<Matrix4x4>(1);
        Global[0] = Director.GetViewProjection();

        // Hand the frame's director to the stages that resolve their draws from it.
        mRenderer.GetPass<Stage::Geometry>(0).SetDirector(Director);
        mRenderer.GetPass<Stage::Light>(1).SetDirector(Director);

        mRenderer.Run(Global.GetStream());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::OnCreate(Ref<Engine::Subsystem::Host> Host, Bool Immediate)
    {
        // Declare the managed targets in the same order as Target.
        Ref<Render::Target> Albedo   = mRenderer.AddTarget({ .Format = Graphic::TextureFormat::RGBA8UIntNorm });
        Ref<Render::Target> Normal   = mRenderer.AddTarget({ .Format = Graphic::TextureFormat::RGBA8UIntNorm });
        Ref<Render::Target> Depth    = mRenderer.AddTarget({ .Format = Graphic::TextureFormat::D24S8UIntNorm });
        Ref<Render::Target> Radiance = mRenderer.AddTarget({ .Format = Graphic::TextureFormat::RGBA16Float   });
        Ref<Render::Target> Final    = mRenderer.AddTarget({ .Format = Graphic::TextureFormat::RGBA8UIntNorm });

        // Geometry: rasterize the scene into the albedo and normal buffers, depth-tested.
        Ref<Stage::Geometry> Geometry = mRenderer.AddPass<Stage::Geometry>(Host, mTileset);
        Geometry.AddColor({ .Target = AddressOf(Albedo), .Tint = Color::Black() });
        Geometry.AddColor({ .Target = AddressOf(Normal), .Tint = Color(0.5f, 0.5f, 1.0f, 1.0f) });
        Geometry.SetDepth({ .Target = AddressOf(Depth) });

        // Light: accumulates each light's contribution into the radiance buffer, sampling the normal buffer.
        Ref<Stage::Light> Light = mRenderer.AddPass<Stage::Light>(Host, Normal);
        Light.AddColor({ .Target = AddressOf(Radiance), .Tint = Color::Black() });

        // Composite: resolves albedo against radiance, into the display when immediate, otherwise into the final buffer.
        Ref<Stage::Composite> Composite = mRenderer.AddPass<Stage::Composite>(Host, Albedo, Radiance);
        Composite.AddColor({ .Target = Immediate ? nullptr : AddressOf(Final), .Load = Graphic::Action::Discard });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::OnRegister(Ref<Scene::Service> Scene)
    {
        // System for ticking the tileset animations based on the absolute time of the scene.
        Scene.CreateSystem<Scene::DSL::In<const Scene::Clock>>(
            "Visual::ComputeTilesetAnimation",
            EcsOnUpdate,
            Scene::Execution::Immediate,
            [this](ConstRef<Scene::Clock> Clock)
            {
                mTileset.Tick(Clock.GetAbsolute());
            });
    }
}
