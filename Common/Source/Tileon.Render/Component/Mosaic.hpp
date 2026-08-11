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

        /// \brief Represents a rectangular run of tiles sharing one terrain.
        struct Block final
        {
            /// The region-local x-coordinate of the block's origin.
            UInt8        X;

            /// The region-local y-coordinate of the block's origin.
            UInt8        Y;

            /// The width of the block, in tiles.
            UInt8        Width;

            /// The height of the block, in tiles.
            UInt8        Height;

            /// The terrain handle shared by every tile of the block.
            UInt16       Handle;

            /// The alignment offset shared by every tile of the block.
            Tile::Offset Offset;
        };

    public:

        /// \brief Construct the mosaic component.
        ZY_INLINE Mosaic()
            : mInvalidated { true }
        {
        }

        /// \brief Marks the cache as out of date, forcing a rebuild before the next draw.
        ZY_INLINE void Invalidate()
        {
            mInvalidated = true;
        }

        /// \brief Checks whether the cache must be rebuilt before it can be drawn.
        ///
        /// \return `true` if the cache is out of date, `false` otherwise.
        ZY_INLINE Bool IsInvalidated() const
        {
            return mInvalidated;
        }

        /// \brief Rebuilds the cached blocks by merging the region's tiles across every layer.
        ///
        /// \param Region The region whose tiles are merged.
        void Rebuild(ConstRef<Region> Region);

        /// \brief Gets the cached blocks of the specified layer.
        ///
        /// \param Layer The layer to retrieve the blocks of.
        /// \return The blocks covering the layer, each tile belonging to exactly one block.
        ZY_INLINE ConstSpan<Block> GetBlocks(Tile::Layer Layer) const
        {
            return mBlocks[Enum::Cast(Layer)];
        }

    public:

        /// \brief Gets where a block's origin falls within the period of the motif it draws.
        ///
        /// \param Origin The block's origin, in absolute world tiles.
        /// \param Period The motif's period, in whole tiles.
        /// \param Offset The block's alignment offset, zero to follow the lattice the world defines.
        /// \return The phase along each axis, always inside the period so it survives being sent as a byte.
        ZY_INLINE static IntVector2 GetPhase(IntVector2 Origin, IntVector2 Period, Tile::Offset Offset)
        {
            return Tile::Align(Origin - IntVector2(Offset), Period);
        }

    private:

        /// \brief Define the cache of blocks.
        using Cache = Array<Sequence<Block>, Enum::Count<Tile::Layer>()>;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Cache mBlocks;
        Bool  mInvalidated;
    };
}