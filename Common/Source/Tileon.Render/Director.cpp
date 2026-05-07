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
        : mZoom    { 1.0f },
          mMode    { Mode::Ortho },
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

            const Real32 FrustumHalfX = (mMode == Mode::Isometric) ? (HalfWidth * 0.5f + HalfHeight) : HalfWidth;
            const Real32 FrustumHalfY = (mMode == Mode::Isometric) ? (HalfWidth * 0.5f + HalfHeight) : HalfHeight;

            mFrustum.Set(Max(Floor(AbsoluteX - FrustumHalfX), Placement::kMinTileX),
                         Max(Floor(AbsoluteY - FrustumHalfY), Placement::kMinTileY),
                         Min( Ceil(AbsoluteX + FrustumHalfX), Placement::kMaxTileX),
                         Min( Ceil(AbsoluteY + FrustumHalfY), Placement::kMaxTileY));
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Director::SetViewport(Real32 Width, Real32 Height)
    {
        mViewport.Set(Width, Height);

        const Real32 HalfWidth  = (mViewport.GetX() * 0.5f) * mZoom;
        const Real32 HalfHeight = (mViewport.GetY() * 0.5f) * mZoom;
        mCamera.SetOrthographic(-HalfWidth, HalfWidth, -HalfHeight, HalfHeight, 0.0f, 1.0f);

        // Apply an additional shear transformation to the projection matrix if the camera is in isometric mode.
        if (mMode == Mode::Isometric)
        {
            static const Matrix4x4 kIsometricShear(
                Vector4( 1.0f, 0.5f, 0.0f, 0.0f),
                Vector4(-1.0f, 0.5f, 0.0f, 0.0f),
                Vector4::UnitZ(),
                Vector4::UnitW());
            mCamera.SetProjectionMatrix(mCamera.GetProjectionMatrix() * kIsometricShear);
        }
    }
}