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
#include <Zyphryon.Math/Vector3.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a skylight component that contains data for simulating outdoor lighting conditions in a scene.
    class Skylight final
    {
    public:

        /// \brief The direction the sun points at when none is given, matching a high afternoon sun.
        static constexpr Vector3 kDefaultDirection = Vector3(0.0f, 1.0f, 0.5f);

        /// \brief Names the curve the composed scene is compressed into the display range with.
        enum class Tonemap : UInt8
        {
            GT7,        ///< Uchimura's Gran Turismo curve, which leaves the midtones where they were authored.
            ACES,       ///< The ACES filmic curve, which contrast and rolls highlights further.
        };

    public:

        /// \brief Default constructor initializing the skylight with default values.
        ZY_INLINE Skylight()
            : mSunDirection { kDefaultDirection },
              mSunTint      { IntColor8(255, 244, 224) },
              mSkyTint      { IntColor8(160, 190, 255) },
              mGroundTint   { IntColor8( 90,  85,  80) },
              mBrightness   { 0.5f },
              mExposure     { 1.0f },
              mTonemap      { Tonemap::GT7 }
        {
        }

        /// \brief Constructs a skylight with the specified parameters.
        ///
        /// \param SunDirection The direction of the sun, defining the angle of sunlight in the scene.
        /// \param SunTint      The tint of direct sunlight in the scene.
        /// \param SkyTint      The tint of ambient light coming from the sky in the scene.
        /// \param GroundTint   The tint of ambient light reflected from the ground in the scene.
        /// \param Brightness   The overall brightness of the environment, defining the intensity of all lighting effects in the scene.
        /// \param Exposure     The exposure the composed scene is scaled by before the tone map compresses it.
        /// \param Tonemap      The curve the composed scene is compressed into the display range with.
        ZY_INLINE Skylight(Vector3 SunDirection, IntColor8 SunTint, IntColor8 SkyTint, IntColor8 GroundTint, Real32 Brightness, Real32 Exposure, Tonemap Tonemap)
            : mSunTint    { SunTint },
              mSkyTint    { SkyTint },
              mGroundTint { GroundTint },
              mBrightness { Brightness },
              mExposure   { Exposure },
              mTonemap    { Tonemap }
        {
            SetSunDirection(SunDirection);
        }

        /// \brief Sets the direction of the sun.
        ///
        /// \param Direction The new direction of the sun.
        ZY_INLINE void SetSunDirection(Vector3 Direction)
        {
            mSunDirection = Vector3::Normalize(Direction.IsAlmostZero() ? kDefaultDirection : Direction);
        }

        /// \brief Gets the current direction of the sun.
        ///
        /// \return The current direction of the sun, normalized.
        ZY_INLINE Vector3 GetSunDirection() const
        {
            return mSunDirection;
        }

        /// \brief Sets the tint of the sun.
        ///
        /// \param Tint The new tint of the sun.
        ZY_INLINE void SetSunTint(IntColor8 Tint)
        {
            mSunTint = Tint;
        }

        /// \brief Gets the current tint of the sun.
        ///
        /// \return The current tint of the sun.
        ZY_INLINE IntColor8 GetSunTint() const
        {
            return mSunTint;
        }

        /// \brief Sets the tint of the sky.
        ///
        /// \param Tint The new tint of the sky.
        ZY_INLINE void SetSkyTint(IntColor8 Tint)
        {
            mSkyTint = Tint;
        }

        /// \brief Gets the current tint of the sky.
        ///
        /// \return The current tint of the sky.
        ZY_INLINE IntColor8 GetSkyTint() const
        {
            return mSkyTint;
        }

        /// \brief Sets the tint of the ground.
        ///
        /// \param Tint The new tint of the ground.
        ZY_INLINE void SetGroundTint(IntColor8 Tint)
        {
            mGroundTint = Tint;
        }

        /// \brief Gets the current tint of the ground.
        ///
        /// \return The current tint of the ground.
        ZY_INLINE IntColor8 GetGroundTint() const
        {
            return mGroundTint;
        }

        /// \brief Sets the overall brightness of the environment.
        ///
        /// \param Brightness The new brightness value for the environment.
        ZY_INLINE void SetBrightness(Real32 Brightness)
        {
            mBrightness = Brightness;
        }

        /// \brief Gets the current overall brightness of the environment.
        ///
        /// \return The current brightness value of the environment.
        ZY_INLINE Real32 GetBrightness() const
        {
            return mBrightness;
        }

        /// \brief Sets the exposure the composed scene is scaled by before the tone map compresses it.
        ///
        /// \param Exposure The new exposure value for the environment.
        ZY_INLINE void SetExposure(Real32 Exposure)
        {
            mExposure = Exposure;
        }

        /// \brief Gets the exposure the composed scene is scaled by before the tone map compresses it.
        ///
        /// \return The current exposure value of the environment.
        ZY_INLINE Real32 GetExposure() const
        {
            return mExposure;
        }

        /// \brief Sets the curve the composed scene is compressed into the display range with.
        ///
        /// \param Tonemap The new curve for the environment.
        ZY_INLINE void SetTonemap(Tonemap Tonemap)
        {
            mTonemap = Tonemap;
        }

        /// \brief Gets the curve the composed scene is compressed into the display range with.
        ///
        /// \return The current curve of the environment.
        ZY_INLINE Tonemap GetTonemap() const
        {
            return mTonemap;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZY_INLINE void Serialize(Serializer Archive)
        {
            Archive.Serialize(mSunDirection);
            Archive.Serialize(mSunTint);
            Archive.Serialize(mSkyTint);
            Archive.Serialize(mGroundTint);
            Archive.Serialize(mBrightness);
            Archive.Serialize(mExposure);
            Archive.Serialize(mTonemap);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Vector3   mSunDirection;
        IntColor8 mSunTint;
        IntColor8 mSkyTint;
        IntColor8 mGroundTint;
        Real32    mBrightness;
        Real32    mExposure;
        Tonemap   mTonemap;
    };
}