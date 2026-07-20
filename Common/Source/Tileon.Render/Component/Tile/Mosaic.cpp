// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Mosaic.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Mosaic::Rebuild(ConstRef<Region> Region, ConstRef<Tileset> Tileset)
    {
        for (const Tile::Layer Layer : Enum::GetValues<Tile::Layer>())
        {
            Ref<Sequence<Block>> Blocks = mBlocks[Enum::Cast(Layer)];
            Blocks.Clear();

            // Tracks, per row, which columns an already emitted block covers.
            Array<UInt32, Region::kTilesPerY> Resolution { };

            for (UInt32 TileY = 0; TileY < Region::kTilesPerY; ++TileY)
            {
                // Skip rows that an earlier block already covers end to end.
                if (Resolution[TileY] == kMaximum<UInt32>)
                {
                    continue;
                }

                for (UInt32 TileX = 0; TileX < Region::kTilesPerX; ++TileX)
                {
                    // Skip tiles already absorbed into a previous block.
                    if (HasBit(Resolution[TileY], 1u << TileX))
                    {
                        continue;
                    }

                    ConstRef<Tile> Tile = Region.GetTile(TileX, TileY);

                    const UInt16 Handle = Tile.GetHandle(Layer);
                    const UInt16 Weight = Tile.GetWeight(Layer);

                    if (Handle == 0)
                    {
                        continue;
                    }

                    const IntVector2 Span = Tileset.GetGlyph(Handle).Span;

                    UInt8 CountX = 1;
                    UInt8 CountY = 1;

                    // A zero span means the tileset has not resolved this terrain yet, so the tile stands
                    // alone for now; the generation check rebuilds the cache once the glyph arrives.
                    if (Span.GetX() > 0 && Span.GetY() > 0)
                    {
                        const UInt32 Stride    = static_cast<UInt32>(Span.GetX());
                        const UInt32 MaxInnerX = Min<UInt32>(Region::kTilesPerX, TileX + (Stride - Weight % Stride));

                        // Try to expand horizontally to adjacent columns that share the same terrain.
                        for (UInt32 InnerX = TileX + 1; InnerX < MaxInnerX; ++InnerX, ++CountX)
                        {
                            if (HasBit(Resolution[TileY], 1u << InnerX))
                            {
                                break;
                            }

                            ConstRef<Tileon::Tile> InnerTile = Region.GetTile(InnerX, TileY);

                            const UInt16 ExpectedWeight = Weight + (InnerX - TileX);

                            if (InnerTile.GetHandle(Layer) != Handle || InnerTile.GetWeight(Layer) != ExpectedWeight)
                            {
                                break;
                            }
                        }

                        // A block spanning the full row would shift by the operand width, which is undefined.
                        const UInt32 Mask = (CountX >= 32) ? kMaximum<UInt32> : (((1u << CountX) - 1) << TileX);
                        Resolution[TileY] |= Mask;

                        // Try to expand vertically to adjacent rows that share the same terrain.
                        UInt16 RowWeight = static_cast<UInt16>(Weight + Stride);

                        for (UInt32 InnerY = TileY + 1; InnerY < Region::kTilesPerY; ++InnerY, ++CountY, RowWeight += Stride)
                        {
                            Bool Merge = true;

                            for (UInt32 InnerX = TileX; InnerX < TileX + CountX; ++InnerX)
                            {
                                if (HasBit(Resolution[InnerY], 1u << InnerX))
                                {
                                    Merge = false;
                                    break;
                                }

                                ConstRef<Tileon::Tile> InnerTile = Region.GetTile(InnerX, InnerY);

                                const UInt16 ExpectedWeight = RowWeight + (InnerX - TileX);

                                if (InnerTile.GetHandle(Layer) != Handle || InnerTile.GetWeight(Layer) != ExpectedWeight)
                                {
                                    Merge = false;
                                    break;
                                }
                            }

                            if (Merge)
                            {
                                Resolution[InnerY] |= Mask;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        Resolution[TileY] |= (1u << TileX);
                    }

                    Blocks.Append(Block {
                        static_cast<UInt8>(TileX),
                        static_cast<UInt8>(TileY),
                        CountX,
                        CountY,
                        Handle,
                        Weight });

                    // Advance past the columns this block just claimed.
                    TileX += CountX - 1;
                }
            }
        }

        mStale = false;
    }
}
