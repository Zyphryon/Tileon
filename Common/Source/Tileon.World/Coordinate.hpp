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

#include "Region.hpp"
#include <Zyphryon.Math/Geometry/Rect.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Provides utility functions for coordinate transformations in the world.
    class Coordinate final
    {
    public:

        /// \brief Bit shift value for converting world tile x-coordinates to region x-coordinates.
        static constexpr SInt32 kBitShiftLocalX = Math::Log(Region::kTilesPerX);

        /// \brief Bit mask for extracting the local x-coordinate within a region.
        static constexpr SInt32 kBitMaskLocalX  = (1u << kBitShiftLocalX) - 1u;

        /// \brief Bit shift value for converting world tile y-coordinates to region y-coordinates.
        static constexpr SInt32 kBitShiftLocalY = Math::Log(Region::kTilesPerY);

        /// \brief Bit mask for extracting the local y-coordinate within a region.
        static constexpr SInt32 kBitMaskLocalY  = (1u << kBitShiftLocalY) - 1u;

    public:

        /// \brief Converts world tile x-coordinates to region x-coordinates.
        ///
        /// \param WorldTileX The x-coordinate in world tile space.
        /// \return The corresponding x-coordinate in region space.
        ZYPHRYON_INLINE static constexpr SInt16 GetRegionX(SInt32 WorldTileX)
        {
            return WorldTileX >> kBitShiftLocalX;
        }

        /// \brief Extracts the local x-coordinate within a region from a world tile x-coordinate.
        ///
        /// \param WorldTileX The x-coordinate in world tile space.
        /// \return The local x-coordinate within the region.
        ZYPHRYON_INLINE static constexpr UInt8 GetLocalTileX(SInt32 WorldTileX)
        {
            return WorldTileX & kBitMaskLocalX;
        }

        /// \brief Converts world tile y-coordinates to region y-coordinates.
        ///
        /// \param WorldTileY The y-coordinate in world tile space.
        /// \return The corresponding y-coordinate in region space.
        ZYPHRYON_INLINE static constexpr SInt16 GetRegionY(SInt32 WorldTileY)
        {
            return WorldTileY >> kBitShiftLocalY;
        }

        /// \brief Extracts the local y-coordinate within a region from a world tile y-coordinate.
        ///
        /// \param WorldTileY The y-coordinate in world tile space.
        /// \return The local y-coordinate within the region.
        ZYPHRYON_INLINE static constexpr UInt8 GetLocalTileY(SInt32 WorldTileY)
        {
            return WorldTileY & kBitMaskLocalY;
        }

        /// \brief Converts world coordinates to cell coordinates based on the specified bit shifts for x and y axes.
        ///
        /// \tparam ShiftX     The bit shift value for the x-axis, determining the size of the cell in world units.
        /// \tparam ShiftY     The bit shift value for the y-axis, determining the size of the cell in world units.
        /// \param Coordinates The rectangle representing the world coordinates to convert.
        /// \return A rect representing the cell coordinates corresponding to the given world coordinates
        template<SInt32 ShiftX, SInt32 ShiftY = ShiftX>
        ZYPHRYON_INLINE static constexpr IntRect GetCell(IntRect Coordinates)
        {
            const SInt32 MinX = Coordinates.GetMinimumX() >> ShiftX;
            const SInt32 MinY = Coordinates.GetMinimumY() >> ShiftY;
            const SInt32 MaxX = (Coordinates.GetMaximumX() + (1 << ShiftX) - 1) >> ShiftX;
            const SInt32 MaxY = (Coordinates.GetMaximumY() + (1 << ShiftY) - 1) >> ShiftY;
            return IntRect(MinX, MinY, MaxX, MaxY);
        }

        /// \brief Converts world coordinates to region cell coordinates.
        ///
        /// \param Coordinates The rectangle representing the world coordinates to convert.
        /// \return A rect representing the region cell coordinates corresponding to the given world coordinates
        ZYPHRYON_INLINE static constexpr IntRect GetRegionCell(IntRect Coordinates)
        {
            return GetCell<kBitShiftLocalX, kBitShiftLocalY>(Coordinates);
        }

        /// \brief Converts world coordinates to cell coordinates based on the specified bit shifts for x and y axes.
        ///
        /// \tparam ShiftX     The bit shift value for the x-axis, determining the size of the cell in world units.
        /// \tparam ShiftY     The bit shift value for the y-axis, determining the size of the cell in world units.
        /// \param Coordinates The rectangle representing the world coordinates to convert.
        /// \return A rect representing the cell coordinates corresponding to the given world coordinates
        template<SInt32 ShiftX, SInt32 ShiftY = ShiftX>
        ZYPHRYON_INLINE static constexpr IntRect GetCell(Rect Coordinates)
        {
            const SInt32 MinX = static_cast<SInt32>(Floor(Coordinates.GetMinimumX())) >> ShiftX;
            const SInt32 MinY = static_cast<SInt32>(Floor(Coordinates.GetMinimumY())) >> ShiftY;
            const SInt32 MaxX = static_cast<SInt32>(-Ceil(Coordinates.GetMaximumX())) >> ShiftX;
            const SInt32 MaxY = static_cast<SInt32>(-Ceil(Coordinates.GetMaximumY())) >> ShiftY;
            return IntRect(MinX, MinY, -MaxX, -MaxY);
        }

        /// \brief Converts world coordinates to region cell coordinates.
        ///
        /// \param Coordinates The rectangle representing the world coordinates to convert.
        /// \return A rect representing the region cell coordinates corresponding to the given world coordinates
        ZYPHRYON_INLINE static constexpr IntRect GetRegionCell(Rect Coordinates)
        {
            return GetCell<kBitShiftLocalX, kBitShiftLocalY>(Coordinates);
        }
    };
}