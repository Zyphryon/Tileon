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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a spot light component that contains runtime data for a spot light entity.
    class Spotlight final
    {
    public:

        /// \brief Default constructor initializing the spot light with default values.
        ZYPHRYON_INLINE Spotlight()
            : mInnerAngle { Angle::FromDegrees(15.0f) },
              mOuterAngle { Angle::FromDegrees(30.0f) },
              mRange      { 10.0f },
              mIntensity  { 1.0f },
              mFalloff    { 1.0f }
        {
        }

        /// \brief Constructs a spot light with the specified parameters.
        ///
        /// \param InnerAngle The inner angle of the spot light, defining the fully illuminated area.
        /// \param OuterAngle The outer angle of the spot light, defining the area where the light starts to fade.
        /// \param Range      The maximum distance the light can reach.
        /// \param Intensity  The brightness of the light at its source.
        /// \param Falloff    The rate at which the light intensity decreases with distance.
        ZYPHRYON_INLINE Spotlight(Angle InnerAngle, Angle OuterAngle, Real32 Range, Real32 Intensity, Real32 Falloff)
            : mRange      { Range },
              mIntensity  { Intensity },
              mFalloff    { Falloff }
        {
            SetAngles(InnerAngle, OuterAngle);
        }

        /// \brief Sets the inner and outer angles of the spot light.
        ///
        /// \param InnerAngle The inner angle of the spot light.
        /// \param OuterAngle The outer angle of the spot light.
        ZYPHRYON_INLINE void SetAngles(Angle InnerAngle, Angle OuterAngle)
        {
            mInnerAngle = InnerAngle;
            mOuterAngle = OuterAngle;
        }

        /// \brief Gets the inner angle of the spot light.
        ///
        /// \return The inner angle of the spot light.
        ZYPHRYON_INLINE Angle GetInnerAngle() const
        {
            return mInnerAngle;
        }

        /// \brief Gets the outer angle of the spot light.
        ///
        /// \return The outer angle of the spot light.
        ZYPHRYON_INLINE Angle GetOuterAngle() const
        {
            return mOuterAngle;
        }

        /// \brief Sets the range of the spot light.
        ///
        /// \param Range The new range value to set for the light.
        ZYPHRYON_INLINE void SetRange(Real32 Range)
        {
            mRange = Range;
        }

        /// \brief Gets the range of the spot light.
        ///
        /// \return The range of the spot light.
        ZYPHRYON_INLINE Real32 GetRange() const
        {
            return mRange;
        }

        /// \brief Sets the intensity of the spot light.
        ///
        /// \param Intensity The new intensity value to set for the light.
        ZYPHRYON_INLINE void SetIntensity(Real32 Intensity)
        {
            mIntensity = Intensity;
        }

        /// \brief Gets the intensity of the spot light.
        ///
        /// \return The intensity of the spot light.
        ZYPHRYON_INLINE Real32 GetIntensity() const
        {
            return mIntensity;
        }

        /// \brief Sets the falloff of the spot light.
        ///
        /// \param Falloff The new falloff value to set for the light.
        ZYPHRYON_INLINE void SetFalloff(Real32 Falloff)
        {
            mFalloff = Falloff;
        }

        /// \brief Gets the falloff of the spot light.
        ///
        /// \return The falloff of the spot light.
        ZYPHRYON_INLINE Real32 GetFalloff() const
        {
            return mFalloff;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
        {
            Archive.SerializeObject(mInnerAngle);
            Archive.SerializeObject(mOuterAngle);
            Archive.SerializeReal32(mRange);
            Archive.SerializeReal32(mIntensity);
            Archive.SerializeReal32(mFalloff);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Angle  mInnerAngle;
        Angle  mOuterAngle;
        Real32 mRange;
        Real32 mIntensity;
        Real32 mFalloff;
    };
}