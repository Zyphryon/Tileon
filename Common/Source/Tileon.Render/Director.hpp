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

#include "Projection.hpp"
#include "Tileon.World/Placement.hpp"
#include <Zyphryon.Math/Motion/Tween.hpp>
#include <Zyphryon.Math/Geometry/Rect.hpp>
#include <Zyphryon.Render/Camera.hpp>

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
        static constexpr Real32 kDelay        = 0.25f;

        /// \brief The minimum zoom value, representing the furthest the camera can zoom in.
        static constexpr Real32 kMinZoom      = 0.125f;

        /// \brief The maximum zoom value, representing the furthest the camera can zoom out.
        static constexpr Real32 kMaxZoom      = 16.0f;

        /// \brief The greatest height the view keeps, in tiles.
        static constexpr Real32 kMaxElevation = 64.0f;

        /// \brief The depth the world begins at, reserving the range in front of it for content drawn over it.
        static constexpr Real32 kMinDepth     = 0.10f;

        /// \brief The depth the world ends at, reserving the range behind it for content drawn under it.
        static constexpr Real32 kMaxDepth     = 0.90f;

    public:

        /// \brief Constructs it with default camera settings.
        Director();

        /// \brief Advances any active position and zoom transitions.
        ///
        /// \param Delta The time delta since the last update, in seconds.
        void Tick(Real64 Delta);

        /// \brief Brings the camera matrices and the frustum and screen rectangles up to date.
        ///
        /// \return `true` if anything changed since the previous refresh, `false` otherwise.
        Bool Compute();

        /// \brief Sets how the world axes project onto the screen.
        ///
        /// \param Projection The projection to view the world through.
        ZY_INLINE void SetProjection(ConstRef<Projection> Projection)
        {
            mProjection = Projection;

            SetViewport(mViewport.GetX(), mViewport.GetY());
        }

        /// \brief Gets how the world axes project onto the screen.
        ///
        /// \return The projection the world is viewed through.
        ZY_INLINE ConstRef<Projection> GetProjection() const
        {
            return mProjection;
        }

        /// \brief Sets the pixel density of the camera's viewport.
        ///
        /// \param Density The pixel density of the camera's viewport, in pixels per logical unit.
        ZY_INLINE void SetDensity(UInt16 Density)
        {
            mDensity = Density;
        }

        /// \brief Gets the pixel density of the camera's viewport.
        ///
        /// \return The pixel density of the camera's viewport, in pixels per logical unit.
        ZY_INLINE UInt16 GetDensity() const
        {
            return mDensity;
        }

        /// \brief Sets the viewport dimensions of the camera.
        ///
        /// \param Width  The width of the viewport in logical units (e.g., world units).
        /// \param Height The height of the viewport in logical units (e.g., world units).
        void SetViewport(Real32 Width, Real32 Height);

        /// \brief Gets the current viewport dimensions of the camera.
        ///
        /// \return The current viewport dimensions of the camera, in logical units (e.g., world units).
        ZY_INLINE Vector2 GetViewport() const
        {
            return mViewport;
        }

        /// \brief Sets the camera's position to a specific placement, immediately applying the change.
        ///
        /// \param Position The new placement to set for the camera.
        ZY_INLINE void SetPosition(Placement Position)
        {
            // Reset any active position tween to immediately apply the new translation.
            mTweenPosition = Tween<Placement>();

            // Update the camera's position to the new placement.
            mPosition = Placement::Clamp(Position);

            // Apply only the sub-region offset to the camera to avoid floating-point precision loss, snapped
            // to the pixel grid so a placement set outright lands where a transition one would.
            mCamera.SetTranslation(Snap(mPosition.GetOffsetX()), 0.0f, Snap(mPosition.GetOffsetY()));
        }

        /// \brief Gets the current placement of the camera.
        ///
        /// \return The current placement of the camera, including region coordinates and sub-region offset.
        ZY_INLINE Placement GetPosition() const
        {
            return mPosition;
        }

        /// \brief Sets the camera's zoom level to a specific magnitude, immediately applying the change.
        ///
        /// \param Magnitude The new zoom level to set for the camera.
        ZY_INLINE void SetZoom(Real32 Magnitude)
        {
            // Reset any active zoom tween to immediately apply the new zoom.
            mTweenZoom = Tween<Real32>();

            mZoom = Clamp(Magnitude, kMinZoom, kMaxZoom);
            SetViewport(mViewport.GetX(), mViewport.GetY());
        }

        /// \brief Gets the current zoom level of the camera.
        ///
        /// \return The current zoom level of the camera.
        ZY_INLINE Real32 GetZoom() const
        {
            return mZoom;
        }

        /// \brief Moves the camera by a specified translation vector.
        ///
        /// \param Delta The translation vector to move the camera by, in logical units (e.g., world units).
        ZY_INLINE void Move(Vector2 Delta)
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
        ZY_INLINE Bool IsMoving() const
        {
            return !mTweenPosition.IsComplete();
        }

        /// \brief Zooms the camera by a specified magnitude.
        ///
        /// \param Magnitude The magnitude to zoom the camera by.
        ZY_INLINE void Zoom(Real32 Magnitude)
        {
            if (mTweenZoom.IsComplete())
            {
                mTweenZoom = Tween(mZoom, Clamp(mZoom + Magnitude, kMinZoom, kMaxZoom), kDelay);
            }
        }

        /// \brief Focuses the camera on a specific anchor point while transitioning to a new zoom level.
        ///
        /// \param Anchor The anchor point to focus on, specified as a placement in the world.
        /// \param Zoom   The new zoom level to transition to while focusing on the anchor point.
        ZY_INLINE void Focus(Placement Anchor, Real32 Zoom)
        {
            if (mTweenZoom.IsComplete() && mTweenPosition.IsComplete())
            {
                const Real32 Magnitude   = Clamp(Zoom, kMinZoom, kMaxZoom);
                const Real32 Scale       = Magnitude / mZoom;

                const Real64 AnchorAbsX  = Anchor.GetAbsoluteX();
                const Real64 AnchorAbsY  = Anchor.GetAbsoluteY();
                const Real64 TargetAbsX  = AnchorAbsX + (mPosition.GetAbsoluteX() - AnchorAbsX) * Scale;
                const Real64 TargetAbsY  = AnchorAbsY + (mPosition.GetAbsoluteY() - AnchorAbsY) * Scale;

                const Placement Target = Placement::FromAbsolute(TargetAbsX, TargetAbsY);
                mTweenPosition = Tween(mPosition, Target, kDelay);
                mTweenZoom     = Tween(mZoom, Magnitude, kDelay);
            }
        }

        /// \brief Checks if the camera is currently zooming due to an active tween.
        ///
        /// \return `true` if the camera is currently zooming, `false` otherwise.
        ZY_INLINE Bool IsZooming() const
        {
            return !mTweenZoom.IsComplete();
        }

        /// \brief Gets the view-projection matrix of the camera.
        ///
        /// \return The view-projection matrix of the camera.
        ZY_INLINE ConstRef<Matrix4x4> GetViewProjection() const
        {
            return mCamera.GetViewProjection();
        }

        /// \brief Gets the inverse of the view-projection matrix of the camera.
        ///
        /// \return The inverse of the view-projection matrix of the camera.
        ZY_INLINE ConstRef<Matrix4x4> GetViewProjectionInverse() const
        {
            return mCamera.GetViewProjectionInverse();
        }

        /// \brief Checks whether a world-space volume covers any of the screen.
        ///
        /// \param Volume The absolute world-space volume to test.
        /// \return `true` when any part of the volume is on screen, `false` otherwise.
        ZY_INLINE Bool IsVisible(ConstRef<IntBox> Volume) const
        {
            const IntVector3 Base = IntVector3::FromXZ(IntVector2(mPosition.GetBaseX(), mPosition.GetBaseY()));
            return mProjection.Project(Volume - Base).Test(mScreen);
        }

        /// \brief Gets the current frustum of the camera's view in logical units (e.g., world units).
        ///
        /// \return The current frustum of the camera's view.
        ZY_INLINE IntRect GetFrustum() const
        {
            return mFrustum;
        }

        /// \brief Converts a world-space offset into the screen-space offset it projects to.
        ///
        /// \param World The world-space offset to project.
        /// \return The corresponding offset in screen pixels.
        ZY_INLINE Vector2 GetScreenDirection(Vector3 World) const
        {
            const Graphic::Viewport Viewport(0, 0, mViewport.GetX(), mViewport.GetY());

            return mCamera.GetScreenDirection<Render::Camera::Origin::Northwest>(World, Viewport) * mDensity;
        }

        /// \brief Converts world coordinates to screen pixel coordinates.
        ///
        /// \param World The world coordinates to convert, including region coordinates and sub-region offset.
        /// \return The corresponding screen pixel coordinates, where (0, 0) is the top-left corner of the viewport.
        ZY_INLINE Vector2 GetScreenCoordinates(Placement World) const
        {
            const Graphic::Viewport Viewport(0, 0, mViewport.GetX(), mViewport.GetY());

            const Vector3 Local = Vector3::FromXZ(
                Vector2(static_cast<Real32>(World.GetAbsoluteX() - mPosition.GetBaseX()),
                        static_cast<Real32>(World.GetAbsoluteY() - mPosition.GetBaseY())));

            const Vector3 Tile = mCamera.GetScreenCoordinates<Render::Camera::Origin::Northwest>(Local, Viewport);
            return Vector2(Tile.GetX(), Tile.GetY()) * mDensity;
        }

        /// \brief Converts screen pixel coordinates to world coordinates.
        ///
        /// \param Pixel The screen pixel coordinates to convert, where (0, 0) is the top-left corner of the viewport.
        /// \return The corresponding world coordinates, including region coordinates and sub-region offset.
        ZY_INLINE Placement GetWorldCoordinates(Vector2 Pixel) const
        {
            const Graphic::Viewport Viewport(0, 0, mViewport.GetX(), mViewport.GetY());

            const Vector3 Tile(Pixel.GetX() / mDensity, Pixel.GetY() / mDensity, 0.0f);
            const Vector3 Local = mCamera.GetWorldCoordinates<Render::Camera::Origin::Northwest>(Tile, Viewport);

            const Vector3 Step   = mProjection.GetPickDirection();
            const Vector2 Ground = (Local - Step * (Local.GetY() / Step.GetY())).GetXZ();

            return Placement::Clamp(Placement(mPosition.GetRegionX(), mPosition.GetRegionY(), Ground.GetX(), Ground.GetY()));
        }

    private:

        /// \brief Snaps a value to the nearest pixel grid based on the current density to prevent sub-pixel artifacts.
        ///
        /// \param Input The value to snap in logical units (e.g., world units).
        /// \return The snapped value aligned to the effective pixel grid.
        template<typename Type>
        ZY_INLINE Type Snap(Type Input) const
        {
            const Real32 Scale = mZoom / mDensity;
            return Round(Input / Scale) * Scale;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Render::Camera   mCamera;
        Real32           mZoom;
        Vector2          mViewport;
        Projection       mProjection;
        UInt16           mDensity;
        IntRect          mFrustum;
        Placement        mPosition;
        Rect             mScreen;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Tween<Placement> mTweenPosition;
        Tween<Real32>    mTweenZoom;
    };
}