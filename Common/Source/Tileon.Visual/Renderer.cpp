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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Visual
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Renderer::Renderer(Ref<Service::Host> Host)
        : Locator(Host),
          mPhases   { },
          mFrames   { },
          mPipeline { Host },
          mTileset  { Host }
    {
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
        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();

        // Destroy existing render targets and passes, as they are no longer valid after a resize.
        for (Ref<Graphic::Object> Phase : mPhases)
        {
            if (Phase)
            {
                Graphics.DeletePass(Phase);

                Phase = 0;
            }
        }
        for (Ref<Graphic::Object> Frame : mFrames)
        {
            if (Frame)
            {
                Graphics.DeleteTexture(Frame);

                Frame = 0;
            }
        }

        // Recreate render targets and passes with the new dimensions.
        mViewport = Graphic::Viewport(0.0f, 0.0f, Width, Height);

        mFrames[Enum::Cast(Frame::Albedo)]   = Graphics.CreateTexture(Graphic::TextureFormat::RGBA8UIntNorm, true, Width, Height);
        mFrames[Enum::Cast(Frame::Normal)]   = Graphics.CreateTexture(Graphic::TextureFormat::RG8UIntNorm,   true, Width, Height);
        mFrames[Enum::Cast(Frame::Depth)]    = Graphics.CreateTexture(Graphic::TextureFormat::D24S8UIntNorm, true, Width, Height);
        mFrames[Enum::Cast(Frame::Radiance)] = Graphics.CreateTexture(Graphic::TextureFormat::RGBA16Float,   true, Width, Height);
        mFrames[Enum::Cast(Frame::Final)]    = Graphics.CreateTexture(Graphic::TextureFormat::RGBA8UIntNorm, true, Width, Height);

        const Graphic::ColorAttachment        GeometryColorAttachments[] = {
            { mFrames[Enum::Cast(Frame::Albedo)], 0, 0, 0, Graphic::Operation::Clear, Graphic::Operation::Store },
            { mFrames[Enum::Cast(Frame::Normal)], 0, 0, 0, Graphic::Operation::Clear, Graphic::Operation::Store }
        };
        const Graphic::DepthStencilAttachment GeometryDepthAttachment(mFrames[Enum::Cast(Frame::Depth)]);
        mPhases[Enum::Cast(Phase::Geometry)] = Graphics.CreatePass(GeometryColorAttachments, GeometryDepthAttachment);

        const Graphic::ColorAttachment LightColorAttachments[] = {
            { mFrames[Enum::Cast(Frame::Radiance)], 0, 0, 0, Graphic::Operation::Clear, Graphic::Operation::Store },
        };
        mPhases[Enum::Cast(Phase::Light)] = Graphics.CreatePass(LightColorAttachments, {});

        const Graphic::ColorAttachment CompositeColorAttachments[] = {
            { mFrames[Enum::Cast(Frame::Final)], 0, 0, 0, Graphic::Operation::Discard, Graphic::Operation::Store },
        };
        mPhases[Enum::Cast(Phase::Composite)] = Graphics.CreatePass(CompositeColorAttachments, {});
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::Present(ConstRef<Director> Director, Bool Immediate)
    {
        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();

        constexpr Array<Color, 2> kClearAlbedoNormal = { Color::Black(), Color(0.5f, 0.5f, 1.0f, 1.0f) };

        Graphics.Prepare(mPhases[Enum::Cast(Phase::Geometry)], mViewport, kClearAlbedoNormal, 1.0f, 0);
        {
            mPipeline.Geometry.Run(Director, mTileset);
        }
        Graphics.Commit();

        Graphics.Prepare(mPhases[Enum::Cast(Phase::Light)], mViewport, Color::Black(), 1.0f, 0);
        {
            mPipeline.Light.Run(mEncoder, Director, GetFrame(Frame::Normal));

            Graphics.Submit(mEncoder);
        }
        Graphics.Commit();

        const Graphic::Object Final = Immediate ? Graphic::kDisplay : mPhases[Enum::Cast(Phase::Composite)];
        Graphics.Prepare(Final, mViewport, Color::Transparent(), 1.0f, 0);
        {
            mPipeline.Composite.Run(mEncoder, GetFrame(Frame::Albedo), GetFrame(Frame::Radiance));

            Graphics.Submit(mEncoder);
        }
        Graphics.Commit();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::OnRegister(Ref<Scene::Service> Scene)
    {
        // System for ticking the tileset animations based on the absolute time of the scene.
        Scene.CreateSystem<Scene::DSL::In<const Time>>(
            "Visual::ComputeTilesetAnimation",
            EcsOnUpdate,
            Scene::Execution::Immediate,
            [this](Time Time)
            {
                mTileset.Tick(Time.GetAbsolute());
            });
    }
}