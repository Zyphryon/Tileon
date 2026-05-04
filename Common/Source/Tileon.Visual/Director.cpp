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

namespace Tileon::Visual
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Director::Director()
        : mZoom    { 1.0f },
          mDensity { 32 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Director::Tick(Real64 Delta)
    {
        // Ensure that the camera's position changes in discrete steps that align with the pixel grid.
        if (!mTweenPosition.IsComplete())
        {
            mPosition = mTweenPosition.Tick(Delta);

            mCamera.SetTranslation(Snap(mPosition.GetOffsetX()), Snap(mPosition.GetOffsetY()));
        }

        // Ensure that the zoom level changes in discrete steps that align with the pixel grid.
        if (!mTweenZoom.IsComplete())
        {
            mZoom = mTweenZoom.Tick(Delta);

            SetViewport(mViewport.GetX(), mViewport.GetY());
        }

        // Compute the camera's transformation and determine if it has changed since the last update.
        const Bool Dirty = mCamera.Compute();

        if (Dirty)
        {
            const Real32 HalfWidth  = (mViewport.GetX() * 0.5f * mZoom);
            const Real32 HalfHeight = (mViewport.GetY() * 0.5f * mZoom);

            const Real64 AbsoluteX  = mPosition.GetAbsoluteX();
            const Real64 AbsoluteY  = mPosition.GetAbsoluteY();

            mFrustum.Set(Max(Floor(AbsoluteX - HalfWidth),  Placement::kMinTileX),
                         Max(Floor(AbsoluteY - HalfHeight), Placement::kMinTileY),
                         Min( Ceil(AbsoluteX + HalfWidth),  Placement::kMaxTileX),
                         Min( Ceil(AbsoluteY + HalfHeight), Placement::kMaxTileY));
        }
        return Dirty;
    }
}