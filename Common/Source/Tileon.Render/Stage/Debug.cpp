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

#include "Debug.hpp"
#include "Tileon.World/Component.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Stage
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Debug::Debug(Ref<Engine::Subsystem::Host> Host)
        : Locator     { Host },
          mProperties { 0 }
    {
        OnRegister(* Host.GetService<Scene::Service>());
        OnLoad(* Host.GetService<Content::Service>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Debug::Run(Ref<Render::Encoder> Encoder)
    {
        if (HasProperty(Property::Grid))
        {
            DrawGrid(Encoder);
        }

        if (HasProperty(Property::Boundaries))
        {
            DrawBoundaries(Encoder);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Debug::DrawGrid(Ref<Render::Encoder> Encoder)
    {
        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();


        Graphic::Transient<GpuGridLayout> Data = Graphics.AllocateTransientUniforms<GpuGridLayout>(1);
        Data[0].Camera    = mDirector->GetViewProjectionInverse();
        Data[0].Dimension = Vector2(Region::kTilesPerX, Region::kTilesPerY);
        Encoder.SetPass(Data.GetStream());

        constexpr Graphic::Invocation Invocation = {
            .Count     = 3,
            .Base      = 0,
            .Offset    = 0,
            .Instances = 1
        };
        Encoder.Draw(* mTechniques[Enum::Cast(Kind::Grid)], ConstSpan<Graphic::Object>(), Invocation);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Debug::DrawBoundaries(Ref<Render::Encoder> Encoder)
    {
        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();
        ConstRef<Director>    Director = (* mDirector);

        // A bound is absolute while the camera is rebased onto its region, so shift each one into the camera's space.
        const IntVector2 Origin(
            Director.GetPosition().GetBaseX(),
            Director.GetPosition().GetBaseY());

        // Both the bound and the frustum are absolute tile rectangles, making the cull a single overlap test.
        const IntRect Frustum = Director.GetFrustum();

        // Reset the boundary data for the current frame.
        mBoundaries.Clear();

        // Accumulate the boundaries of the visible entities and render them in a single batch.
        mQrDrawBoundaries.Run<const Bound>([&](ConstRef<Bound> Bounds)
        {
            const IntRect AABB = Bounds.GetRect();

            if (!AABB.Test(Frustum))
            {
                return;
            }

            const Vector2 Minimum(
                static_cast<Real32>(AABB.GetMinimumX() - Origin.GetX()),
                static_cast<Real32>(AABB.GetMinimumY() - Origin.GetY()));
            const Vector2 Maximum(
                static_cast<Real32>(AABB.GetMaximumX() - Origin.GetX()),
                static_cast<Real32>(AABB.GetMaximumY() - Origin.GetY()));

            mBoundaries.Append((Minimum + Maximum) * 0.5f, (Maximum - Minimum) * 0.5f);
        });

        if (const ConstSpan<GpuBoundaryLayout> Data = mBoundaries; !Data.IsEmpty())
        {
            Graphic::Transient<GpuBoundaryLayout> Instances
                = Graphics.AllocateTransientVertices<GpuBoundaryLayout>(Data.GetSize());
            Instances.Copy(Data);

            const Graphic::Invocation Invocation = {
                .Count     = 4,
                .Base      = 0,
                .Offset    = 0,
                .Instances = static_cast<UInt32>(Data.GetSize())
            };
            Encoder.Draw(* mTechniques[Enum::Cast(Kind::Boundary)], { }, Instances.GetStream(), Invocation);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Debug::OnRegister(Ref<Scene::Service> Scene)
    {
        // Creates the queries for the debug stage.
        mQrDrawBoundaries = Scene.CreateQuery<
            Scene::DSL::In<const Bound>
        >("Render::Debug::DrawBoundaries", Scene::Cache::Auto);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Debug::OnLoad(Ref<Content::Service> Content)
    {
        for (const Kind Type : Enum::GetValues<Kind>())
        {
            Str Path = Str::Print<"Resources://Technique/Debug/{0}.vfx">(Enum::GetName(Type));

            mTechniques[Enum::Cast(Type)] = Content.Load<Graphic::Technique>(Move(Path));
        }
    }
}
