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
    /// \brief Represents the runtime profile of the application, containing display and performance parameters.
    class Profile final
    {
    public:

        /// \brief Constructs a profile with default display parameters.
        Profile();

        /// \brief Sets the display parameters for the application.
        ///
        /// \param Width   The width of the display in pixels.
        /// \param Height  The height of the display in pixels.
        /// \param Density The pixel density of the display (e.g., pixels per logical unit).
        ZYPHRYON_INLINE void SetDisplay(UInt16 Width, UInt16 Height, UInt16 Density)
        {
            mDisplayWidth   = Width;
            mDisplayHeight  = Height;
            mDisplayDensity = Density;
        }

        /// \brief Gets the width of the display in pixels.
        ///
        /// \return The width of the display in pixels.
        ZYPHRYON_INLINE UInt16 GetDisplayWidth() const
        {
            return mDisplayWidth;
        }

        /// \brief Gets the height of the display in pixels.
        ///
        /// \return The height of the display in pixels.
        ZYPHRYON_INLINE UInt16 GetDisplayHeight() const
        {
            return mDisplayHeight;
        }

        /// \brief Gets the pixel density of the display (e.g., pixels per logical unit).
        ///
        /// \return The pixel density of the display.
        ZYPHRYON_INLINE UInt16 GetDisplayDensity() const
        {
            return mDisplayDensity;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UInt16 mDisplayWidth;
        UInt16 mDisplayHeight;
        UInt16 mDisplayDensity;
    };
}