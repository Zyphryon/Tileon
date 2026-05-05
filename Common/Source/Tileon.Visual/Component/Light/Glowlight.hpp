// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a glow light component that contains runtime data for a glow light entity.
    class Glowlight final
    {
    public:

        /// \brief Default constructor initializing the glow light with default values.
        ZYPHRYON_INLINE Glowlight()
            : mRadius    { 10.0f },
              mIntensity { 1.0f },
              mFalloff   { 1.0f }
        {
        }

        /// \brief Constructs a glow light with the specified parameters.
        ///
        /// \param Radius    The radius of the light, defining the area of effect.
        /// \param Intensity The intensity of the light, defining its brightness.
        /// \param Falloff   The falloff of the light, defining how quickly the intensity diminishes with distance.
        ZYPHRYON_INLINE Glowlight(Real32 Radius, Real32 Intensity, Real32 Falloff)
            : mRadius    { Radius },
              mIntensity { Intensity },
              mFalloff   { Falloff }
        {
        }

        /// \brief Sets the radius of the glow light.
        ///
        /// \param Radius The new radius value to set for the light.
        ZYPHRYON_INLINE void SetRadius(Real32 Radius)
        {
            mRadius = Radius;
        }

        /// \brief Gets the radius of the glow light.
        ///
        /// \return The current radius value of the light.
        ZYPHRYON_INLINE Real32 GetRadius() const
        {
            return mRadius;
        }

        /// \brief Sets the intensity of the glow light.
        ///
        /// \param Intensity The new intensity value to set for the light.
        ZYPHRYON_INLINE void SetIntensity(Real32 Intensity)
        {
            mIntensity = Intensity;
        }

        /// \brief Gets the intensity of the glow light.
        ///
        /// \return The current intensity value of the light.
        ZYPHRYON_INLINE Real32 GetIntensity() const
        {
            return mIntensity;
        }

        /// \brief Sets the falloff of the glow light.
        ///
        /// \param Falloff The new falloff value to set for the light.
        ZYPHRYON_INLINE void SetFalloff(Real32 Falloff)
        {
            mFalloff = Falloff;
        }

        /// \brief Gets the falloff of the glow light.
        ///
        /// \return The current falloff value of the light.
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
            Archive.SerializeReal32(mRadius);
            Archive.SerializeReal32(mIntensity);
            Archive.SerializeReal32(mFalloff);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Real32 mRadius;
        Real32 mIntensity;
        Real32 mFalloff;
    };
}
