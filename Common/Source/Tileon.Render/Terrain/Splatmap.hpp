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

#include "Tileon.World/Component/Region.hpp"
#include <Zyphryon.Math/Geometry/Rect.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents the ground of a region, as slices of the shared array and how much each of them shows.
    class Splatmap final
    {
    public:

        /// \brief The number of terrains the ground of a region blends between.
        static constexpr UInt8  kSlots      = 4;

        /// \brief The page of the weight array a region carries before the stage has lent it one.
        static constexpr UInt16 kUnassigned = kMaximum<UInt16>;

    public:

        /// \brief Constructs a bare splatmap, with nothing painted anywhere.
        ZY_INLINE Splatmap()
            : mPage  { kUnassigned },
              mFlags { Flag::Invalidated }
        {
        }

        /// \brief Sets the slice a slot draws for the whole region.
        ///
        /// \param Slot  The slot to assign, below \ref kSlots.
        /// \param Slice The slice the slot draws, of the shared array.
        ZY_INLINE void SetSplat(UInt8 Slot, UInt16 Slice)
        {
            mSplats[Slot] = Slice;
        }

        /// \brief Gets the slice a slot draws.
        ///
        /// \param Slot The slot to read, below \ref kSlots.
        /// \return The slice the slot draws.
        ZY_INLINE UInt16 GetSplat(UInt8 Slot) const
        {
            return mSplats[Slot];
        }

        /// \brief Gets every slice the ground blends between, in slot order.
        ///
        /// \return The palette of the region, one slice per slot.
        ZY_INLINE ConstSpan<UInt16> GetSplats() const
        {
            return mSplats;
        }

        /// \brief Sets how much each slot shows through at one point on the ground.
        ///
        /// \note Does not invalidate the map, the caller marks the edit with \ref Invalidate once it is done.
        ///
        /// \param X       The x-coordinate of the point within the region.
        /// \param Y       The y-coordinate of the point within the region.
        /// \param Weights The weight each slot carries there, which the caller is expected to have normalized.
        ZY_INLINE void SetWeights(UInt8 X, UInt8 Y, ConstSpan<UInt8> Weights)
        {
            const UInt32 Offset = ConvertTo1D<UInt32>(X, Y, Region::kUnitsPerX) * kSlots;

            for (UInt8 Slot = 0; Slot < kSlots; ++Slot)
            {
                mWeights[Offset + Slot] = Weights[Slot];
            }
        }

        /// \brief Gets how much a slot shows through at one point on the ground.
        ///
        /// \param X    The x-coordinate of the point within the region.
        /// \param Y    The y-coordinate of the point within the region.
        /// \param Slot The slot to read, below \ref kSlots.
        /// \return The weight the slot carries there.
        ZY_INLINE UInt8 GetWeight(UInt8 X, UInt8 Y, UInt8 Slot) const
        {
            return mWeights[ConvertTo1D<UInt32>(X, Y, Region::kUnitsPerX) * kSlots + Slot];
        }

        /// \brief Gets the whole weight map, laid out as the texture the ground samples.
        ///
        /// \return The region's weights, one quadruple per unit in row order.
        ZY_INLINE ConstSpan<UInt8> GetWeights() const
        {
            return mWeights;
        }

        /// \brief Checks whether any of the ground still carries a slot's weight.
        ///
        /// \param Slot The slot to look for, below \ref kSlots.
        /// \return `true` when the slot shows anywhere, `false` when nothing draws it.
        ZY_INLINE Bool IsVisible(UInt8 Slot) const
        {
            for (UInt32 Index = Slot; Index < mWeights.GetSize(); Index += kSlots)
            {
                if (mWeights[Index] > 0)
                {
                    return true;
                }
            }
            return false;
        }

        /// \brief Claims the slot a slice is drawn from, taking a free one when it has none yet.
        ///
        /// \param Slice The slice to claim a slot for, of the shared array.
        /// \return The slot the slice is drawn from, or \ref kSlots when the palette is full.
        ZY_INLINE UInt8 Claim(UInt16 Slice)
        {
            for (UInt8 Slot = 0; Slot < kSlots; ++Slot)
            {
                if (mSplats[Slot] == Slice && IsVisible(Slot))
                {
                    return Slot;
                }
            }

            for (UInt8 Slot = 0; Slot < kSlots; ++Slot)
            {
                if (!IsVisible(Slot))
                {
                    mSplats[Slot] = Slice;
                    return Slot;
                }
            }
            return kSlots;
        }

        /// \brief Blends a slot into one point of the ground, pushing the other slots aside by as much.
        ///
        /// \note Does not invalidate the map, the caller marks the edit with \ref Invalidate once it is done.
        ///
        /// \param X        The x-coordinate of the point within the region.
        /// \param Y        The y-coordinate of the point within the region.
        /// \param Slot     The slot to blend in, below \ref kSlots.
        /// \param Strength The share of the point the slot takes, from `0` for none to `255` for all of it.
        ZY_INLINE void Blend(UInt8 X, UInt8 Y, UInt8 Slot, UInt8 Strength)
        {
            const UInt32 Offset = ConvertTo1D<UInt32>(X, Y, Region::kUnitsPerX) * kSlots;
            const UInt32 Rest   = 255 - Strength;

            for (UInt8 Index = 0; Index < kSlots; ++Index)
            {
                mWeights[Offset + Index] = static_cast<UInt8>(mWeights[Offset + Index] * Rest / 255);
            }

            const UInt32 Total = mWeights[Offset + Slot] + Strength;
            mWeights[Offset + Slot] = static_cast<UInt8>(Total < 255 ? Total : 255);
        }

        /// \brief Paints a slice across an area of the ground, claiming a slot for it if it has none.
        ///
        /// \note Does not invalidate the map, the caller marks the edit with \ref Invalidate once it is done.
        ///
        /// \param Area  The area of the region to paint, in region-local coordinates.
        /// \param Slice The slice to paint, of the shared array.
        /// \return `true` when the area was painted, `false` when the palette had no slot left to give.
        ZY_INLINE Bool Paint(IntRect Area, UInt16 Slice)
        {
            const UInt8 Chosen = Claim(Slice);

            if (Chosen == kSlots)
            {
                return false;
            }

            for (SInt32 Y = Area.GetMinimumY(); Y < Area.GetMaximumY(); ++Y)
            {
                for (SInt32 X = Area.GetMinimumX(); X < Area.GetMaximumX(); ++X)
                {
                    Blend(static_cast<UInt8>(X), static_cast<UInt8>(Y), Chosen, 255);
                }
            }
            return true;
        }

        /// \brief Takes a page of the weight array, which invalidates whatever was written there before.
        ///
        /// \param Page The page the map of the region now occupies.
        ZY_INLINE void Assign(UInt16 Page)
        {
            if (mPage != Page)
            {
                mPage  = Page;
                mFlags = SetBit(mFlags, Flag::Invalidated);
            }
        }

        /// \brief Gives the page back, so that another region may take it.
        ZY_INLINE void Release()
        {
            mPage  = kUnassigned;
            mFlags = SetBit(mFlags, Flag::Invalidated);
        }

        /// \brief Gets the page of the weight array the map of the region occupies.
        ///
        /// \return The page, or \ref kUnassigned when the region has none.
        ZY_INLINE UInt16 GetPage() const
        {
            return mPage;
        }

        /// \brief Marks the weights as changed by an edit of this region, which spreads to its neighbours.
        ZY_INLINE void Invalidate()
        {
            Refresh();

            mFlags = SetBit(mFlags, Flag::Spread);
        }

        /// \brief Records that the weights now match what the GPU holds.
        ZY_INLINE void Validate()
        {
            mFlags = ClearBit(mFlags, Flag::Invalidated);
        }

        /// \brief Checks whether the weights have yet to reach the GPU.
        ///
        /// \return `true` while the map on the GPU is older than the one held here.
        ZY_INLINE Bool IsInvalidated() const
        {
            return HasBit(mFlags, Flag::Invalidated);
        }

        /// \brief Marks the weights as changed by an edit of a neighbour, which spreads no further.
        ZY_INLINE void Refresh()
        {
            mFlags = SetBit(mFlags, Flag::Invalidated);
            mFlags = SetOrClearBit(mFlags, Flag::Painted, Measure());
        }

        /// \brief Checks whether any of the ground has been painted at all.
        ///
        /// \return `true` when something is drawn somewhere, as read when the weights last changed.
        ZY_INLINE Bool IsPainted() const
        {
            return HasBit(mFlags, Flag::Painted);
        }

        /// \brief Records that the neighbours have been told, so the edit spreads no further.
        ZY_INLINE void Settle()
        {
            mFlags = ClearBit(mFlags, Flag::Spread);
        }

        /// \brief Checks whether the edit still has neighbours to dirty.
        ///
        /// \return `true` until the neighbours have been told their own gutters moved.
        ZY_INLINE Bool IsSpreading() const
        {
            return HasBit(mFlags, Flag::Spread);
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZY_INLINE void Serialize(Serializer Archive)
        {
            Archive.Serialize(mSplats);
            Archive.Serialize(mWeights);
        }

    private:

        /// \brief Specifies what is worth remembering about the map between one frame and the next.
        enum class Flag : UInt8
        {
            Invalidated = 0b00000001,   ///< The weights have yet to reach the GPU.
            Painted     = 0b00000010,   ///< Something is drawn somewhere, as read when they last changed.
            Spread      = 0b00000100,   ///< The edit still has neighbours to dirty.
        };
        ZY_DEFINE_BITWISE_FRIEND_ENUM(Flag)

        /// \brief Walks every weight to answer whether any of the ground has been painted at all.
        ///
        /// \return `true` when something is drawn somewhere, `false` when the region is bare.
        ZY_INLINE Bool Measure() const
        {
            for (UInt32 Offset = 0; Offset < mWeights.GetSize(); Offset += sizeof(UInt64))
            {
                UInt64 Chunk;
                Blit(AddressOf(Chunk), sizeof(Chunk), mWeights.GetData() + Offset);

                if (Chunk != 0)
                {
                    return true;
                }
            }
            return false;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Array<UInt16, kSlots>                          mSplats;
        Array<UInt8, Region::kUnitsPerRegion * kSlots> mWeights;
        UInt16                                         mPage;
        Flag                                           mFlags;
    };
}