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

#include <Zyphryon.Math/Color.hpp>
#include <Zyphryon.Math/Vector2.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Visual
{
    /// \brief Represents the component that contains runtime data for the lighting and ambient conditions of a scene.
    class Environment final
    {
    public:

        /// \brief Default constructor initializing the environment with default values.
        ZYPHRYON_INLINE Environment()
            : mSunDirection { Vector2::UnitY() },
              mSunTint      { IntColor8::White() },
              mSkyTint      { IntColor8::White() },
              mGroundTint   { IntColor8::White() },
              mBrightness   { 1.0f }
        {
        }

        /// \brief Constructs an environment with the specified parameters.
        ///
        /// \param SunDirection The direction of the sun, defining the angle of sunlight in the scene.
        /// \param SunTint      The tint of direct sunlight in the scene.
        /// \param SkyTint      The tint of ambient light coming from the sky in the scene.
        /// \param GroundTint   The tint of ambient light reflected from the ground in the scene.
        /// \param Brightness   The overall brightness of the environment, defining the intensity of all lighting effects in the scene.
        ZYPHRYON_INLINE Environment(Vector2 SunDirection, IntColor8 SunTint, IntColor8 SkyTint, IntColor8 GroundTint, Real32 Brightness)
            : mSunDirection { SunDirection },
              mSunTint      { SunTint },
              mSkyTint      { SkyTint },
              mGroundTint   { GroundTint },
              mBrightness   { Brightness }
        {
        }

        /// \brief Sets the direction of the sun.
        ///
        /// \param Direction The new direction of the sun.
        ZYPHRYON_INLINE void SetSunDirection(Vector2 Direction)
        {
            mSunDirection = Direction;
        }

        /// \brief Gets the current direction of the sun.
        ///
        /// \return The current direction of the sun.
        ZYPHRYON_INLINE Vector2 GetSunDirection() const
        {
            return mSunDirection;
        }

        /// \brief Sets the tint of the sun.
        ///
        /// \param Tint The new tint of the sun.
        ZYPHRYON_INLINE void SetSunTint(IntColor8 Tint)
        {
            mSunTint = Tint;
        }

        /// \brief Gets the current tint of the sun.
        ///
        /// \return The current tint of the sun.
        ZYPHRYON_INLINE IntColor8 GetSunTint() const
        {
            return mSunTint;
        }

        /// \brief Sets the tint of the sky.
        ///
        /// \param Tint The new tint of the sky.
        ZYPHRYON_INLINE void SetSkyTint(IntColor8 Tint)
        {
            mSkyTint = Tint;
        }

        /// \brief Gets the current tint of the sky.
        ///
        /// \return The current tint of the sky.
        ZYPHRYON_INLINE IntColor8 GetSkyTint() const
        {
            return mSkyTint;
        }

        /// \brief Sets the tint of the ground.
        ///
        /// \param Tint The new tint of the ground.
        ZYPHRYON_INLINE void SetGroundTint(IntColor8 Tint)
        {
            mGroundTint = Tint;
        }

        /// \brief Gets the current tint of the ground.
        ///
        /// \return The current tint of the ground.
        ZYPHRYON_INLINE IntColor8 GetGroundTint() const
        {
            return mGroundTint;
        }

        /// \brief Sets the overall brightness of the environment.
        ///
        /// \param Brightness The new brightness value for the environment.
        ZYPHRYON_INLINE void SetBrightness(Real32 Brightness)
        {
            mBrightness = Brightness;
        }

        /// \brief Gets the current overall brightness of the environment.
        ///
        /// \return The current brightness value of the environment.
        ZYPHRYON_INLINE Real32 GetBrightness() const
        {
            return mBrightness;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
        {
            Archive.SerializeObject(mSunDirection);
            Archive.SerializeObject(mSunTint);
            Archive.SerializeObject(mSkyTint);
            Archive.SerializeObject(mGroundTint);
            Archive.SerializeReal32(mBrightness);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Vector2   mSunDirection;
        IntColor8 mSunTint;
        IntColor8 mSkyTint;
        IntColor8 mGroundTint;
        Real32    mBrightness;
    };
}
