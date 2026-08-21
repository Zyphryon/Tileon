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

#include <Zyphryon.Math/Angle.hpp>
#include <Zyphryon.Math/Matrix4x4.hpp>
#include <Zyphryon.Math/Geometry/Box.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Describes how the three world axes project onto the screen.
    ///
    /// The world is a ground plane on XZ with Y as elevation, each axis contributes a fixed offset per world unit.
    class Projection final
    {
    public:

        /// \brief Constructs a projection that collapses every axis onto a single point.
        ZY_INLINE constexpr Projection() = default;

        /// \brief Constructs a projection from the screen offset each world axis contributes.
        ///
        /// \param AxisX The screen offset per unit of world X, one of the two ground axes.
        /// \param AxisY The screen offset per unit of world Y, which is elevation.
        /// \param AxisZ The screen offset per unit of world Z, the other ground axis.
        ZY_INLINE constexpr Projection(Vector2 AxisX, Vector2 AxisY, Vector2 AxisZ)
            : mAxisX { AxisX },
              mAxisY { AxisY },
              mAxisZ { AxisZ }
        {
        }

        /// \brief Checks whether the ground and elevation project onto separate screen axes.
        ///
        /// \return `true` when world X drives only the screen's horizontal and world Y and Z only its vertical.
        ZY_INLINE constexpr Bool IsAxisAligned() const
        {
            return IsAlmostZero(mAxisX.GetY()) && IsAlmostZero(mAxisY.GetX()) && IsAlmostZero(mAxisZ.GetX());
        }

        /// \brief Gets the screen offset contributed by one unit of world X.
        ///
        /// \return The screen offset per unit of world X.
        ZY_INLINE constexpr Vector2 GetAxisX() const
        {
            return mAxisX;
        }

        /// \brief Gets the screen offset contributed by one unit of elevation.
        ///
        /// \return The screen offset per unit of world Y.
        ZY_INLINE constexpr Vector2 GetAxisY() const
        {
            return mAxisY;
        }

        /// \brief Gets the screen offset contributed by one unit of world Z.
        ///
        /// \return The screen offset per unit of world Z.
        ZY_INLINE constexpr Vector2 GetAxisZ() const
        {
            return mAxisZ;
        }

        /// \brief Builds the matrix that shears world space onto the screen.
        ///
        /// \return The shear to compose onto an orthographic projection.
        ZY_INLINE Matrix4x4 GetShear() const
        {
            return Matrix4x4(mAxisX.GetX(), mAxisX.GetY(), mAxisX.GetY(), 0.0f,
                             mAxisY.GetX(), mAxisY.GetY(), 0.0f,          0.0f,
                             mAxisZ.GetX(), mAxisZ.GetY(), mAxisZ.GetY(), 0.0f,
                             0.0f,          0.0f,          0.0f,          1.0f);
        }

        /// \brief Gets the depth a point sorts at, before the camera maps it into the buffer's range.
        ///
        /// \param World The world-space point to measure.
        /// \return The unmapped depth, increasing away from the viewer.
        ZY_INLINE constexpr Real32 GetDepth(Vector3 World) const
        {
            return mAxisX.GetY() * World.GetX() + mAxisZ.GetY() * World.GetZ();
        }

        /// \brief Gets how far the depth axis reaches for a given ground half-extent and elevation.
        ///
        /// \param Ground    The half-extent of the ground the screen covers, along world X and world Z.
        /// \param Elevation The greatest height the view has to keep.
        /// \return The half-extent of the depth the screen covers.
        ZY_INLINE constexpr Real32 GetDepthExtent(Vector2 Ground, Real32 Elevation) const
        {
            return Abs(mAxisX.GetY()) * Ground.GetX()
                 + Abs(mAxisZ.GetY()) * Ground.GetY()
                 + Abs(mAxisY.GetY()) * Elevation;
        }

        /// \brief Gets the world-space direction a pick ray travels under this projection.
        ///
        /// \return The direction, in world space, that a pick ray travels.
        ZY_INLINE constexpr Vector3 GetPickDirection() const
        {
            const Real32 InvDeterminant = 1.0f / GetGroundDeterminant();

            const Real32 X = (mAxisZ.GetX() * mAxisY.GetY() - mAxisY.GetX() * mAxisZ.GetY()) * InvDeterminant;
            const Real32 Z = (mAxisY.GetX() * mAxisX.GetY() - mAxisX.GetX() * mAxisY.GetY()) * InvDeterminant;

            return Vector3(X, 1.0f, Z);
        }

        /// \brief Gets the ground the screen covers, as a half-extent about the point the camera sits on.
        ///
        /// \param Half The half-extent of the screen, in world units.
        /// \return The half-extent of the ground the screen covers, along world X and world Z.
        ZY_INLINE constexpr Vector2 GetGroundExtent(Vector2 Half) const
        {
            const Real32 InvDeterminant = 1.0f / GetGroundDeterminant();

            const Real32 X = (Abs(mAxisZ.GetY()) * Half.GetX() + Abs(mAxisZ.GetX()) * Half.GetY()) * InvDeterminant;
            const Real32 Z = (Abs(mAxisX.GetY()) * Half.GetX() + Abs(mAxisX.GetX()) * Half.GetY()) * InvDeterminant;

            return Vector2(X, Z);
        }

        /// \brief Projects a point onto the screen.
        ///
        /// \param World The world-space point to project.
        /// \return The screen offset the point lands at, in world units.
        ZY_INLINE constexpr Vector2 Project(Vector3 World) const
        {
            return mAxisX * World.GetX() + mAxisY * World.GetY() + mAxisZ * World.GetZ();
        }

        /// \brief Projects a volume onto the screen.
        ///
        /// \param Volume The world-space volume to project.
        /// \return The screen area the volume covers, in world units.
        ZY_INLINE constexpr Rect Project(ConstRef<IntBox> Volume) const
        {
            const Vector3 Lower(Volume.GetMinimum());
            const Vector3 Upper(Volume.GetMaximum());

            const Vector2 FirstX  = mAxisX * Lower.GetX();
            const Vector2 SecondX = mAxisX * Upper.GetX();
            const Vector2 FirstY  = mAxisY * Lower.GetY();
            const Vector2 SecondY = mAxisY * Upper.GetY();
            const Vector2 FirstZ  = mAxisZ * Lower.GetZ();
            const Vector2 SecondZ = mAxisZ * Upper.GetZ();

            const Vector2 Minimum = Vector2::Min(FirstX, SecondX)
                                  + Vector2::Min(FirstY, SecondY)
                                  + Vector2::Min(FirstZ, SecondZ);
            const Vector2 Maximum = Vector2::Max(FirstX, SecondX)
                                  + Vector2::Max(FirstY, SecondY)
                                  + Vector2::Max(FirstZ, SecondZ);

            return Rect(Minimum.GetX(), Minimum.GetY(), Maximum.GetX(), Maximum.GetY());
        }

    private:

        /// \brief Gets the determinant of the two ground axes.
        ///
        /// \return The determinant, which is non-zero whenever the ground axes are not parallel.
        ZY_INLINE constexpr Real32 GetGroundDeterminant() const
        {
            const Real32 Determinant = mAxisX.GetX() * mAxisZ.GetY() - mAxisZ.GetX() * mAxisX.GetY();
            ZY_ASSERT(Determinant != 0.0f, "Ground axes must not be parallel");

            return Determinant;
        }

    public:

        /// \brief Creates the projection that views the ground straight on.
        ///
        /// \return The orthographic projection.
        ZY_INLINE static constexpr Projection Ortho()
        {
            return Projection(Vector2(1.0f, 0.0f), Vector2(0.0f, 1.0f), Vector2(0.0f, 1.0f));
        }

        /// \brief Creates the projection that views the ground at a two-to-one isometric angle.
        ///
        /// \return The isometric projection.
        ZY_INLINE static constexpr Projection Isometric()
        {
            return Projection(Vector2(1.0f, 0.5f), Vector2(0.0f, 1.0f), Vector2(-1.0f, 0.5f));
        }

        /// \brief Creates the general axonometric projection.
        ///
        /// \note Generalizes \ref Ortho and \ref Isometric, which are fixed choices of facing and tilt.
        ///
        /// \param Facing The heading the ground is swung to.
        /// \param Tilt   How much the ground is foreshortened, from edge-on (0) to flat on (1).
        /// \return The axonometric projection.
        ZY_INLINE static Projection Axonometric(Angle Facing, Real32 Tilt)
        {
            const Real32 Cosine = Angle::Cosine(Facing);
            const Real32 Sine   = Angle::Sine(Facing);
            return Projection(Vector2(Cosine, Tilt * Sine), Vector2(0.0f, 1.0f), Vector2(-Sine, Tilt * Cosine));
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Vector2 mAxisX;
        Vector2 mAxisY;
        Vector2 mAxisZ;
    };
}