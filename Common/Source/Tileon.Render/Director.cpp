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

#include "Director.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Director::Director()
        : mZoom       { 1.0f },
          mProjection { Projection::Ortho() },
          mDensity    { 32 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Director::Tick(Real64 Delta)
    {
        // Ensure that the camera's position changes in discrete steps that align with the pixel grid.
        if (!mTweenPosition.IsComplete())
        {
            mPosition = mTweenPosition.Tick(Delta);

            mCamera.SetTranslation(Snap(mPosition.GetOffsetX()), 0.0f, Snap(mPosition.GetOffsetY()));
        }

        // Ensure that the zoom level changes in discrete steps that align with the pixel grid.
        if (!mTweenZoom.IsComplete())
        {
            mZoom = mTweenZoom.Tick(Delta);

            SetViewport(mViewport.GetX(), mViewport.GetY());
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Director::Compute()
    {
        if (!mCamera.Compute())
        {
            return false;
        }

        const Real32 HalfWidth  = (mViewport.GetX() * 0.5f * mZoom);
        const Real32 HalfHeight = (mViewport.GetY() * 0.5f * mZoom);

        const Real64 AbsoluteX  = mPosition.GetAbsoluteX();
        const Real64 AbsoluteY  = mPosition.GetAbsoluteY();

        const Vector2 Extent = mProjection.GetGroundExtent(Vector2(HalfWidth, HalfHeight));

        mFrustum.Set(Max(Floor(AbsoluteX - Extent.GetX()), Placement::kMinTileX),
                     Max(Floor(AbsoluteY - Extent.GetY()), Placement::kMinTileY),
                     Min( Ceil(AbsoluteX + Extent.GetX()), Placement::kMaxTileX),
                     Min( Ceil(AbsoluteY + Extent.GetY()), Placement::kMaxTileY));

        const Vector2 Offset(mPosition.GetOffsetX(), mPosition.GetOffsetY());
        const Vector2 Origin = mProjection.Project(Vector3::FromXZ(Offset));

        mScreen.Set(Origin.GetX() - HalfWidth, Origin.GetY() - HalfHeight,
                    Origin.GetX() + HalfWidth, Origin.GetY() + HalfHeight);

        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Director::SetViewport(Real32 Width, Real32 Height)
    {
        mViewport.Set(Width, Height);

        const Real32 HalfWidth  = (mViewport.GetX() * 0.5f) * mZoom;
        const Real32 HalfHeight = (mViewport.GetY() * 0.5f) * mZoom;

        // Depth reserves a band of the buffer so the layers either side of the world keep their own room.
        const Vector2 Ground = mProjection.GetGroundExtent(Vector2(HalfWidth, HalfHeight));
        const Real32  Extent = mProjection.GetDepthExtent(Ground, kMaxHeight);
        const Real32  Span   = (2.0f * Extent) / (kMaxDepth - kMinDepth);
        const Real32  Near   = -Extent - kMinDepth * Span;

        mCamera.SetOrthographic(-HalfWidth, HalfWidth, -HalfHeight, HalfHeight, Near, Near + Span);
        mCamera.SetProjection(mCamera.GetProjection() * mProjection.GetShear());
    }
}