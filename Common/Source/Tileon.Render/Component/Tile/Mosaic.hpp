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

#include "Tileon.Render/Tileset.hpp"
#include "Tileon.World/Region.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Caches the merged tile blocks of a region, so the greedy merge runs on edit instead of every frame.
    class Mosaic final
    {
    public:

        /// \brief Represents a rectangular run of tiles sharing one terrain and consecutive atlas weights.
        struct Block final
        {
            /// The region-local x-coordinate of the block's origin.
            UInt8  X;

            /// The region-local y-coordinate of the block's origin.
            UInt8  Y;

            /// The width of the block, in tiles.
            UInt8  Width;

            /// The height of the block, in tiles.
            UInt8  Height;

            /// The terrain handle shared by every tile of the block.
            UInt16 Handle;

            /// The atlas weight of the block's first tile.
            UInt16 Weight;
        };

    public:

        /// \brief Construct the mosaic component.
        ZY_INLINE Mosaic()
            : mStale { true }
        {
        }

        /// \brief Marks the cache as out of date, forcing a rebuild before the next draw.
        ZY_INLINE void Invalidate()
        {
            mStale = true;
        }

        /// \brief Checks whether the cache must be rebuilt before it can be drawn.
        ///
        /// \return `true` if the cache is out of date, `false` otherwise.
        ZY_INLINE Bool IsStale() const
        {
            return mStale;
        }

        /// \brief Rebuilds the cached blocks by merging the region's tiles across every layer.
        ///
        /// \param Region  The region whose tiles are merged.
        /// \param Tileset The tileset supplying the atlas span of each terrain.
        void Rebuild(ConstRef<Region> Region, ConstRef<Tileset> Tileset);

        /// \brief Gets the cached blocks of the specified layer.
        ///
        /// \param Layer The layer to retrieve the blocks of.
        /// \return The blocks covering the layer, each tile belonging to exactly one block.
        ZY_INLINE ConstSpan<Block> GetBlocks(Tile::Layer Layer) const
        {
            return mBlocks[Enum::Cast(Layer)];
        }

    private:

        /// \brief Define the cache of blocks.
        using Cache = Array<Sequence<Block>, Enum::Count<Tile::Layer>()>;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Cache mBlocks;
        Bool  mStale;
    };
}
