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

#include <Zyphryon.Math/Quaternion.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents the velocity of an entity in the world.
    class Velocity final
    {
    public:

        /// \brief Default constructor initializing linear and angular velocity to zero.
        ZY_INLINE Velocity() = default;

        /// \brief Constructs a velocity with the specified linear and angular velocities.
        ///
        /// \param Linear  The linear velocity vector, in world units per second.
        /// \param Angular The angular velocity, in radians per second about each axis.
        ZY_INLINE Velocity(Vector3 Linear, Vector3 Angular)
            : mLinear  { Linear },
              mAngular { Angular }
        {
        }

        /// \brief Sets the linear velocity of the entity.
        ///
        /// \param Linear The linear velocity to set.
        ZY_INLINE void SetLinear(Vector3 Linear)
        {
            mLinear = Linear;
        }

        /// \brief Gets the linear velocity of the entity.
        ///
        /// \return The linear velocity.
        ZY_INLINE Vector3 GetLinear() const
        {
            return mLinear;
        }

        /// \brief Sets the angular velocity of the entity.
        ///
        /// \param Angular The angular velocity to set, in radians per second about each axis.
        ZY_INLINE void SetAngular(Vector3 Angular)
        {
            mAngular = Angular;
        }

        /// \brief Gets the angular velocity of the entity.
        ///
        /// \return The angular velocity, in radians per second about each axis.
        ZY_INLINE Vector3 GetAngular() const
        {
            return mAngular;
        }

        /// \brief Computes the rotation produced by the angular velocity over the given time span.
        ///
        /// \param Delta The elapsed time in seconds.
        /// \return The rotation accumulated over \p Delta, or the identity rotation when the rate is zero.
        ZY_INLINE Quaternion Integrate(Real32 Delta) const
        {
            const Vector3 Rotation = mAngular * Delta;
            const Real32  Magnitude = Rotation.GetLength();

            if (IsAlmostZero(Magnitude))
            {
                return Quaternion();
            }
            return Quaternion::FromAngles(Angle(Magnitude), Rotation / Magnitude);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Vector3 mLinear;
        Vector3 mAngular;
    };
}