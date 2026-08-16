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

    void Mosaic::Rebuild(ConstRef<Region> Region)
    {
        for (const Tile::Layer Layer : Enum::GetValues<Tile::Layer>())
        {
            Ref<Sequence<Block>> Blocks = mBlocks[Enum::Cast(Layer)];
            Blocks.Clear();

            // One bit per tile, marking the cells an earlier block of this layer has already absorbed.
            Array<UInt32, Region::kTilesPerY> Resolution { };

            for (UInt32 TileY = 0; TileY < Region::kTilesPerY; ++TileY)
            {
                for (UInt32 TileX = 0; TileX < Region::kTilesPerX; ++TileX)
                {
                    if (HasBit(Resolution[TileY], 1u << TileX))
                    {
                        continue;
                    }

                    Tile Anchor = Region.GetTile(TileX, TileY);

                    const UInt16            Handle = Anchor.GetHandle(Layer);
                    const Tile::Offset      Offset = Anchor.GetOffset(Layer);
                    const Tile::Orientation Facing = Anchor.GetOrientation(Layer);

                    if (Handle == 0)
                    {
                        continue;
                    }

                    UInt8 CountX = 1;

                    for (UInt32 InnerX = TileX + 1; InnerX < Region::kTilesPerX; ++InnerX, ++CountX)
                    {
                        const Tile Tile = Region.GetTile(InnerX, TileY);

                        if (HasBit(Resolution[TileY], 1u << InnerX)
                            || Tile.GetHandle(Layer) != Handle
                            || Tile.GetOffset(Layer) != Offset
                            || Tile.GetOrientation(Layer) != Facing)
                        {
                            break;
                        }
                    }

                    // A block spanning the full row would shift by the operand width, which is undefined.
                    const UInt32 Mask = (CountX >= 32) ? kMaximum<UInt32> : (((1u << CountX) - 1) << TileX);
                    Resolution[TileY] |= Mask;

                    // Then take whole rows below, so the block stays the rectangle an instance can draw.
                    UInt8 CountY = 1;

                    for (UInt32 InnerY = TileY + 1; InnerY < Region::kTilesPerY; ++InnerY, ++CountY)
                    {
                        Bool Merge = true;

                        for (UInt32 InnerX = TileX; InnerX < TileX + CountX && Merge; ++InnerX)
                        {
                            const Tile Tile = Region.GetTile(InnerX, InnerY);

                            Merge = !HasBit(Resolution[InnerY], 1u << InnerX)
                                && Tile.GetHandle(Layer) == Handle
                                && Tile.GetOffset(Layer) == Offset
                                && Tile.GetOrientation(Layer) == Facing;
                        }

                        if (!Merge)
                        {
                            break;
                        }
                        Resolution[InnerY] |= Mask;
                    }

                    Blocks.Append(
                        static_cast<UInt8>(TileX),
                        static_cast<UInt8>(TileY), CountX, CountY, Handle, Offset, Facing);

                    TileX += CountX - 1;
                }
            }
        }
        mInvalidated = false;
    }
}