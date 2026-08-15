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

#include "Common.hpp"
#include "Tileon.Render/Tileset.hpp"
#include "Zyphryon.Render/Encoder.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Records the tiles of a pass and drains them as one instanced draw per technique and texture.
    class Tiles final
    {
    public:

        /// \brief Constructs a tile batcher allocating from the specified service.
        ///
        /// \param Service The service the transient instance streams are allocated from.
        explicit Tiles(ConstRetainer<Graphic::Service> Service);

        /// \brief Sets the technique subsequent tiles are recorded under.
        ///
        /// \param Technique The technique every tile up to the next flush is drawn with.
        /// \param Variant   The bitmask of the features they are drawn with.
        void SetTechnique(ConstRetainer<Graphic::Technique> Technique, Graphic::Technique::Key Variant = 0);

        /// \brief Records a tile lying flat on the ground plane.
        ///
        /// \param Glyph    The glyph the tile shows.
        /// \param Phase    Where the tile's origin falls within the glyph's period, from \ref Mosaic::GetPhase.
        /// \param Position The position of the tile's origin, in whole tiles relative to the camera.
        /// \param Size     The span the tile covers on the ground, in whole tiles.
        /// \param Layer    The layer the tile stacks on, where higher layers are drawn in front.
        void Draw(ConstRef<Tileset::Glyph> Glyph, IntVector2 Phase, IntVector2 Position, IntVector2 Size, UInt8 Layer);

        /// \brief Retires the batches recorded so far, keeping their storage for the next pass to draw into.
        void Reset();

        /// \brief Writes each batch as a single instanced command through the encoder.
        ///
        /// \param Encoder The encoder that builds the resulting draw commands.
        void Flush(Ref<Render::Encoder> Encoder);

    private:

        /// \brief Defines a structure representing the input data for drawing a tile in the GPU.
        struct TileLayout final
        {
            /// The position of the tile's origin, in whole tiles relative to the camera.
            Array<SInt16, 2> Position;

            /// The tile's span in whole tiles, the layer it stacks on, and one byte to spare.
            Array<UInt8, 4>  Metrics;

            /// The glyph's period in whole tiles, then where the tile's origin falls within it.
            Array<UInt8, 4>  Lattice;

            /// The slice of the array texture the tile samples.
            UInt32           Slice;

            /// Color tint to apply to the tile, represented as an 8-bit integer color (RGBA).
            IntColor8        Color;
        };

        /// \brief Gathers every tile sampling one array into a single instanced draw.
        struct TileBatch final
        {
            /// The array texture every tile of the batch samples, each from its own slice.
            Graphic::Object      Texture;

            /// The per-instance input data of the batch's tiles.
            Sequence<TileLayout> Layouts;
        };

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Retainer<Graphic::Service>   mService;
        Retainer<Graphic::Technique> mTechnique;
        Graphic::Technique::Key      mVariant;
        Sequence<TileBatch>          mBatches;
        UInt32                       mCount;
    };
}