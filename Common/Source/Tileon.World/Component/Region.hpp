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
    /// \brief Identifies a square span of the world and the coordinate frame everything inside it is local to.
    class Region final
    {
    public:

        /// \brief The number of world units per column in a region.
        static constexpr UInt8  kUnitsPerX      = 32;

        /// \brief The number of world units per row in a region.
        static constexpr UInt8  kUnitsPerY      = 32;

        /// \brief The total number of world units in a region.
        static constexpr UInt16 kUnitsPerRegion = kUnitsPerX * kUnitsPerY;

    public:

        /// \brief Default constructor initializing the region at coordinates (0, 0).
        ZY_INLINE Region()
            : mX { 0 },
              mY { 0 }
        {
        }

        /// \brief Constructs a region at the specified coordinates.
        ///
        /// \param X The x-coordinate of the region.
        /// \param Y The y-coordinate of the region.
        ZY_INLINE Region(SInt16 X, SInt16 Y)
            : mX { X },
              mY { Y }
        {
        }

        /// \brief Gets the x-coordinate of the region.
        ///
        /// \return The x-coordinate of the region.
        ZY_INLINE SInt16 GetX() const
        {
            return mX;
        }

        /// \brief Gets the y-coordinate of the region.
        ///
        /// \return The y-coordinate of the region.
        ZY_INLINE SInt16 GetY() const
        {
            return mY;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        SInt16 mX;
        SInt16 mY;
    };
}