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

#include "Splatmap.hpp"
#include "Splatset.hpp"
#include <Zyphryon.Render/Encoder.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Blends the ground of every region the view reaches, in one draw.
    class Splatter final
    {
    public:

        /// \brief Constructs a splatter drawing the specified terrains.
        ///
        /// \param Service  The graphic service the weight array and instance stream are allocated from.
        /// \param Splatset The terrains the ground blends between.
        /// \param Density  How many pixels of art the project maps onto one world unit.
        Splatter(ConstRetainer<Graphic::Service> Service, Ref<Splatset> Splatset, Real32 Density);

        /// \brief Records one region's ground, whether or not it is going to be drawn.
        ///
        /// \param Region  The region being recorded.
        /// \param Splat   The ground painted on it.
        /// \param Visible Whether the view reaches the region.
        void Record(ConstRef<Region> Region, Ref<Splatmap> Splat, Bool Visible);

        /// \brief Writes whatever pages have changed and draws every region recorded, then forgets them all.
        ///
        /// \param Encoder   The encoder that builds and binds the resulting draw command.
        /// \param Technique The technique the ground blends with.
        /// \param Origin    The whole-unit origin the frame's world coordinates are expressed against.
        void Draw(Ref<Render::Encoder> Encoder, ConstRetainer<Graphic::Technique> Technique, IntVector3 Origin);

        /// \brief Takes back the page a region was lent, leaving it free for another to take.
        ///
        /// \param Splat The ground giving its page back.
        void Release(Ref<Splatmap> Splat);

    private:

        /// \brief The number of region weight maps one array holds, and therefore what one draw reaches.
        static constexpr UInt16 kPage      = 256;

        /// \brief The ring of neighbouring weights every page carries, so a blend crosses a region boundary.
        static constexpr UInt16 kMapBorder = 1;

        /// \brief The side of one region's page, in texels, gutter included.
        static constexpr UInt16 kMapSize   = Region::kUnitsPerX + 2 * kMapBorder;

        /// \brief Represents the per-instance data for one region's ground.
        struct Layout final
        {
            /// The region's corner, in world units, relative to the frame's origin.
            IntVector2                          Origin;

            /// The page of the weight array holding the region's map.
            UInt32                              Weights;

            /// The slice of the terrain array each of the four slots draws.
            Array<UInt16, Splatmap::kSlots>     Palette;

            /// How often each slot repeats across one world unit.
            Array<Real32, Splatmap::kSlots>     Mapping;

            /// Where each slot's terrain already stands in its own repeat at the region's corner.
            Array<Real32, Splatmap::kSlots * 2> Phase;

            /// The color each slot's art is multiplied by.
            Array<IntColor8, Splatmap::kSlots>  Tint;

            /// How wide a band each slot's relief feathers over where it meets another.
            Array<Real32, Splatmap::kSlots>     Feather;
        };

        /// \brief Holds a region and its ground for as long as the frame is being built.
        struct Entry final
        {
            /// The region itself, which names the ground and places it in the world.
            ConstPtr<Region>  Component;

            /// The ground painted on the region, written to as its page is lent and filled.
            Ptr<Splatmap>     Splat;

            /// Whether the region has ground to draw and the view reaches it.
            Bool              Drawable;
        };

        /// \brief Packs a region's coordinates into the key its neighbours are found by.
        ///
        /// \param X The x-coordinate of the region.
        /// \param Y The y-coordinate of the region.
        /// \return The key that names the region.
        ZY_INLINE static constexpr UInt32 GetKey(SInt32 X, SInt32 Y)
        {
            return (static_cast<UInt32>(static_cast<UInt16>(X)) << 16) | static_cast<UInt16>(Y);
        }

        /// \brief Makes one more weight array, and hands its pages to whoever asks next.
        void Allocate();

        /// \brief Tells every neighbour of an edited region that its own gutter has moved.
        ///
        /// \note Builds the index \ref Upload finds its neighbours by, which lives as long as the frame.
        void Spread();

        /// \brief Writes one region's page, gutter and all, into the payload the GPU takes.
        ///
        /// \param Region The region whose page is being written.
        /// \param Splat  The ground painted on it, marked as matching the GPU once it is written.
        void Upload(ConstRef<Region> Region, Ref<Splatmap> Splat);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Retainer<Graphic::Service> mService;
        Ref<Splatset>              mSplatset;
        Real32                     mDensity;
        Sequence<Graphic::Object>  mTextures;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Sequence<UInt16>           mAvailable;
        Sequence<Entry>            mRegions;
        Table<UInt32, UInt32>      mIndices;
        Bool                       mRebuilding;
    };
}