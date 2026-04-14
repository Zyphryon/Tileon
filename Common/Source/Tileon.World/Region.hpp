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

#include "Tile.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a region in the world, consisting of a grid of tiles.
    class Region final
    {
    public:

        /// \brief Number of tiles per row in a region.
        static constexpr UInt8  kTilesPerRow     = 32;

        /// \brief Number of tiles per column in a region.
        static constexpr UInt8  kTilesPerColumn  = 32;

        /// \brief Total number of tiles in a region.
        static constexpr SInt16 kTilesPerRegion  = kTilesPerRow * kTilesPerColumn;

        /// \brief Total number of pixels per row in a region.
        static constexpr SInt32 kPixelsPerRow    = Tile::kExtent * kTilesPerRow;

        /// \brief Total number of pixels per column in a region.
        static constexpr SInt32 kPixelsPerColumn = Tile::kExtent * kTilesPerColumn;

    public:

        /// \brief Default constructor initializing the region at coordinates (0, 0).
        ZYPHRYON_INLINE Region()
            : mX { 0 },
              mY { 0 }
        {
        }

        /// \brief Constructs a region at the specified coordinates.
        ///
        /// \param X The x-coordinate of the region.
        /// \param Y The y-coordinate of the region.
        ZYPHRYON_INLINE Region(SInt16 X, SInt16 Y)
            : mX { X },
              mY { Y }
        {
        }

        /// \brief Gets the x-coordinate of the region.
        ///
        /// \return The x-coordinate of the region.
        ZYPHRYON_INLINE SInt16 GetX() const
        {
            return mX;
        }

        /// \brief Gets the y-coordinate of the region.
        ///
        /// \return The y-coordinate of the region.
        ZYPHRYON_INLINE SInt16 GetY() const
        {
            return mY;
        }

        /// \brief Sets a tile at the specified coordinates within the region.
        ///
        /// \param X    The x-coordinate of the tile within the region.
        /// \param Y    The y-coordinate of the tile within the region.
        /// \param Tile The tile to set at the specified coordinates.
        ZYPHRYON_INLINE void SetTile(UInt8 X, UInt8 Y, AnyRef<Tile> Tile)
        {
            mTiles[Index2D(X, Y, kTilesPerRow)] = Move(Tile);
        }

        /// \brief Gets a tile at the specified coordinates within the region.
        ///
        /// \param X The x-coordinate of the tile within the region.
        /// \param Y The y-coordinate of the tile within the region.
        /// \return A constant reference to the tile at the specified coordinates.
        ZYPHRYON_INLINE ConstRef<Tile> GetTile(UInt8 X, UInt8 Y) const
        {
            return mTiles[Index2D(X, Y, kTilesPerRow)];
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
        {
            Archive.SerializeInt(mX);
            Archive.SerializeInt(mY);
            Archive.SerializeArray(mTiles);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        SInt16                       mX;
        SInt16                       mY;
        Array<Tile, kTilesPerRegion> mTiles;
    };
}