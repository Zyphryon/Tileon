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

#include <Zyphryon.Math/Vector2.hpp>

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
        ZYPHRYON_INLINE Velocity() = default;

        /// \brief Constructs a velocity with the specified linear and angular velocities.
        ///
        /// \param Linear  The linear velocity vector, representing movement in the x and y directions.
        /// \param Angular The angular velocity, representing rotation.
        ZYPHRYON_INLINE Velocity(Vector2 Linear, Angle Angular)
            : mLinear  { Linear },
              mAngular { Angular }
        {
        }

        /// \brief Sets the linear velocity of the entity.
        ///
        /// \param Linear The linear velocity to set.
        ZYPHRYON_INLINE void SetLinear(Vector2 Linear)
        {
            mLinear = Linear;
        }

        /// \brief Gets the linear velocity of the entity.
        ///
        /// \return The linear velocity.
        ZYPHRYON_INLINE Vector2 GetLinear() const
        {
            return mLinear;
        }

        /// \brief Sets the angular velocity of the entity.
        ///
        /// \param Angular The angular velocity to set.
        ZYPHRYON_INLINE void SetAngular(Angle Angular)
        {
            mAngular = Angular;
        }

        /// \brief Gets the angular velocity of the entity.
        ///
        /// \return The angular velocity.
        ZYPHRYON_INLINE Angle GetAngular() const
        {
            return mAngular;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
        {
            Archive.SerializeObject(mLinear);
            Archive.SerializeObject(mAngular);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Vector2 mLinear;
        Angle   mAngular;
    };
}
