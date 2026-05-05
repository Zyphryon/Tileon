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
#include <Zyphryon.Math/Geometry/Rect.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a region in the world, consisting of a grid of tiles.
    class Region final
    {
    public:

        /// \brief Number of tiles per column in a region.
        static constexpr UInt8  kTilesPerX      = 32;

        /// \brief Number of tiles per row in a region.
        static constexpr UInt8  kTilesPerY      = 32;

        /// \brief Total number of tiles in a region.
        static constexpr UInt16 kTilesPerRegion = kTilesPerX * kTilesPerY;

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
            mTiles[Index2D(X, Y, kTilesPerX)] = Move(Tile);
        }

        /// \brief Gets a tile at the specified coordinates within the region.
        ///
        /// \param X The x-coordinate of the tile within the region.
        /// \param Y The y-coordinate of the tile within the region.
        /// \return A constant reference to the tile at the specified coordinates.
        ZYPHRYON_INLINE ConstRef<Tile> GetTile(UInt8 X, UInt8 Y) const
        {
            return mTiles[Index2D(X, Y, kTilesPerX)];
        }

        /// \brief Stamps a terrain onto a rectangular area of tiles on the specified layer.
        ///
        /// \param Area   The rectangular area within the region to fill (region-local coordinates).
        /// \param Layer  The tile layer to fill the terrain on.
        /// \param Handle The unique identifier for the terrain type to stamp onto the tiles.
        /// \param Span   The dimensions of the terrain atlas in tiles, used for calculating the atlas index.
        /// \param Offset The starting offset within the terrain atlas in tiles, used for calculating the atlas index.
        ZYPHRYON_INLINE void Fill(IntRect Area, Tile::Layer Layer, UInt16 Handle, IntVector2 Span, IntVector2 Offset)
        {
            UInt8 AtlasY = Offset.GetY();

            for (UInt8 Y = Area.GetMinimumY(); Y < Area.GetMaximumY(); ++Y)
            {
                UInt8 AtlasX = Offset.GetX();

                for (UInt8 X = Area.GetMinimumX(); X < Area.GetMaximumX(); ++X)
                {
                    mTiles[Index2D(X, Y, kTilesPerX)].SetLayer(Layer, Handle, AtlasY * Span.GetX() + AtlasX);

                    if (++AtlasX >= Span.GetX())
                    {
                        AtlasX = 0;
                    }
                }

                if (++AtlasY >= Span.GetY())
                {
                    AtlasY = 0;
                }
            }
        }

        /// \brief Erases the specified layer of each tile within the rectangular area.
        ///
        /// \param Area  The rectangular area within the region to erase (region-local coordinates).
        /// \param Layer The tile layer to erase from the tiles.
        ZYPHRYON_INLINE void Erase(IntRect Area, Tile::Layer Layer)
        {
            for (UInt8 Y = Area.GetMinimumY(); Y < Area.GetMaximumY(); ++Y)
            {
                for (UInt8 X = Area.GetMinimumX(); X < Area.GetMaximumX(); ++X)
                {
                    mTiles[Index2D(X, Y, kTilesPerX)].SetLayer(Layer, 0, 0);
                }
            }
        }

        /// \brief Resets all layers of every tile within the rectangular area to their default state.
        ///
        /// \param Area The rectangular area within the region to clear (region-local coordinates).
        ZYPHRYON_INLINE void Clear(IntRect Area)
        {
            for (UInt8 Y = Area.GetMinimumY(); Y < Area.GetMaximumY(); ++Y)
            {
                for (UInt8 X = Area.GetMinimumX(); X < Area.GetMaximumX(); ++X)
                {
                    SetTile(X, Y, Tile());
                }
            }
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