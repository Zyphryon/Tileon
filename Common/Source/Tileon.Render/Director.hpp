// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Tileon.World/Placement.hpp"
#include <Zyphryon.Math/Tween.hpp>
#include <Zyphryon.Math/Geometry/Rect.hpp>
#include <Zyphryon.Graphic/Camera.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Encapsulates the camera system for rendering the world, managing view transformations and transitions.
    class Director final
    {
    public:

        /// \brief The delay duration for camera movements and zoom transitions, in seconds.
        static constexpr Real32 kDelay   = 0.25f;

        /// \brief The minimum zoom value, representing the furthest the camera can zoom in.
        static constexpr Real32 kMinZoom = 0.125f;

        /// \brief The maximum zoom value, representing the furthest the camera can zoom out.
        static constexpr Real32 kMaxZoom = 16.0f;

    public:

        /// \brief Constructs it with default camera settings.
        Director();

        /// \brief Updates the camera's state, applying any active tweens or transitions.
        ///
        /// \param Delta The time delta since the last update, in seconds.
        /// \return `true` if the camera's state was updated, `false` otherwise
        Bool Tick(Real64 Delta);

        /// \brief Sets the viewport dimensions and updates the camera's orthographic projection accordingly.
        ///
        /// \param Width   The width of the viewport in logical units (e.g., world units).
        /// \param Height  The height of the viewport in logical units (e.g., world units).
        /// \param Density The pixel density of the viewport (e.g., pixels per logical unit).
        ZYPHRYON_INLINE void SetViewport(UInt16 Width, UInt16 Height, UInt16 Density)
        {
            mViewport.Set(Width, Height);

            mDensity = Density;

            const Real32 HalfWidth  = (static_cast<Real32>(mViewport.GetX()) * 0.5f) * mZoom;
            const Real32 HalfHeight = (static_cast<Real32>(mViewport.GetY()) * 0.5f) * mZoom;
            mCamera.SetOrthographic(-HalfWidth, HalfWidth, -HalfHeight, HalfHeight, 0.0f, 1.0f);
        }

        /// \brief Gets the view-projection matrix of the camera.
        ///
        /// \return The view-projection matrix of the camera.
        ZYPHRYON_INLINE ConstRef<Matrix4x4> GetProjection() const
        {
            return mCamera.GetViewProjection();
        }

        /// \brief Sets the camera's position to a specific placement, immediately applying the change.
        ///
        /// \param Position The new placement to set for the camera.
        ZYPHRYON_INLINE void SetPosition(Placement Position)
        {
            // Reset any active position tween to immediately apply the new translation.
            mTweenPosition = Tween<Placement>();

            // Update the camera's position to the new placement.
            mPosition = Placement::Clamp(Position);

            // Apply only the sub-region offset to the camera to avoid floating-point precision loss.
            mCamera.SetTranslation(
                Snap(mPosition.GetOffsetX()),
                Snap(mPosition.GetOffsetY()));
        }

        /// \brief Gets the current placement of the camera.
        ///
        /// \return The current placement of the camera, including region coordinates and sub-region offset.
        ZYPHRYON_INLINE Placement GetPosition() const
        {
            return mPosition;
        }

        /// \brief Sets the camera's zoom level to a specific magnitude, immediately applying the change.
        ///
        /// \param Magnitude The new zoom level to set for the camera.
        ZYPHRYON_INLINE void SetZoom(Real32 Magnitude)
        {
            // Reset any active zoom tween to immediately apply the new zoom.
            mTweenZoom = Tween<Real32>();

            mZoom = Clamp(Magnitude, kMinZoom, kMaxZoom);
            SetViewport(mViewport.GetX(), mViewport.GetY(), mDensity);
        }

        /// \brief Gets the current zoom level of the camera.
        ///
        /// \return The current zoom level of the camera.
        ZYPHRYON_INLINE Real32 GetZoom() const
        {
            return mZoom;
        }

        /// \brief Moves the camera by a specified translation vector.
        ///
        /// \param Delta The translation vector to move the camera by, in logical units (e.g., world units).
        ZYPHRYON_INLINE void Move(Vector2 Delta)
        {
            if (mTweenPosition.IsComplete())
            {
                const Placement Target(
                    mPosition.GetRegionX(),
                    mPosition.GetRegionY(),
                    mPosition.GetOffsetX() + Delta.GetX(),
                    mPosition.GetOffsetY() + Delta.GetY());

                mTweenPosition = Tween(mPosition, Placement::Clamp(Target), kDelay);
            }
        }

        /// \brief Checks if the camera is currently moving due to an active tween.
        ///
        /// \return `true` if the camera is currently moving, `false` otherwise.
        ZYPHRYON_INLINE Bool IsMoving() const
        {
            return !mTweenPosition.IsComplete();
        }

        /// \brief Zooms the camera by a specified magnitude.
        ///
        /// \param Magnitude The magnitude to zoom the camera by.
        ZYPHRYON_INLINE void Zoom(Real32 Magnitude)
        {
            if (mTweenZoom.IsComplete())
            {
                mTweenZoom = Tween(mZoom, Clamp(mZoom + Magnitude, kMinZoom, kMaxZoom), kDelay);
            }
        }

        /// \brief Checks if the camera is currently zooming due to an active tween.
        ///
        /// \return `true` if the camera is currently zooming, `false` otherwise.
        ZYPHRYON_INLINE Bool IsZooming() const
        {
            return !mTweenZoom.IsComplete();
        }

        /// \brief Gets the current frustum of the camera's view in logical units (e.g., world units).
        ///
        /// \return The current frustum of the camera's view.
        ZYPHRYON_INLINE IntRect GetFrustum() const
        {
            return mFrustum;
        }

    private:

        /// \brief Snaps a value to the nearest pixel grid based on the current density to prevent sub-pixel artifacts.
        ///
        /// \param Input The value to snap in logical units (e.g., world units).
        /// return The snapped value aligned to the effective pixel grid.
        template<typename Type>
        ZYPHRYON_INLINE Type Snap(Type Input) const
        {
            const Real32 Scale = mZoom / mDensity;
            return Round(Input / Scale) * Scale;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Graphic::Camera  mCamera;
        Real32           mZoom;
        UIntVector2      mViewport;
        UInt16           mDensity;
        IntRect          mFrustum;
        Placement        mPosition;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Tween<Placement> mTweenPosition;
        Tween<Real32>    mTweenZoom;
    };
}
