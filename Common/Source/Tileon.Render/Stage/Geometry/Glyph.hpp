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

#include "Tileon.Render/Typography/Contour.hpp"
#include "Tileon.Render/Typography/Label.hpp"
#include "Tileon.Render/Typography/Typeface.hpp"
#include "Zyphryon.Render/Collector.hpp"
#include "Zyphryon.Render/Encoder.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Batch
{
    /// \brief Records the text of a pass and writes back the batches the collector hands it.
    class Glyph final
    {
    public:

        /// \brief Constructs a glyph batcher allocating from the specified service.
        ///
        /// \param Service   The service the transient streams are allocated from.
        /// \param Collector The collector the glyphs are ordered and batched by.
        /// \param Kind      The tag every draw is stamped with, so the drain routes its batches back here.
        Glyph(ConstRetainer<Graphic::Service> Service, Ref<Render::Collector> Collector, UInt32 Kind);

        /// \brief Sets the technique subsequent glyphs are recorded under.
        ///
        /// \param Technique The technique to set for subsequent glyphs.
        void SetTechnique(ConstRetainer<Graphic::Technique> Technique);

        /// \brief Records a run of text with the specified parameters.
        ///
        /// \param Face      The typeface supplying the font the glyphs come from and the size they are set at.
        /// \param Label     The label supplying the content to draw and the spacing it is laid out with.
        /// \param Transform The transformation matrix to apply to the text for positioning, scaling, and rotation.
        /// \param Effect    The contour supplying the effect the glyphs are drawn with.
        /// \param Tint      The tint color to apply to the glyphs.
        void Draw(ConstRef<Typeface> Face, ConstRef<Label> Label, ConstRef<Matrix4x3> Transform, ConstRef<Contour> Effect, IntColor8 Tint);

        /// \brief Drops the glyphs and runs recorded so far.
        void Reset();

        /// \brief Uploads the run palettes the pass's glyphs index into.
        void Prepare();

        /// \brief Writes one batch of glyphs as a single instanced command through the encoder.
        ///
        /// \param Encoder  The encoder that builds the resulting draw command.
        /// \param Commands The span of draw calls forming the batch.
        void Write(Ref<Render::Encoder> Encoder, ConstSpan<Render::Collector::Command> Commands);

    private:

        /// \brief The maximum number of text runs one draw call can batch together.
        static constexpr UInt32 kMaxTextPerBatch = 128;

        /// \brief Represents the per-run data a batch's glyphs index into, in the layout the shaders consume.
        struct Uniform final
        {
            /// The transform the run is placed by, with the text's size folded into the basis.
            Array<Real32, 12>  Transform;

            /// The effect every glyph of the run is drawn with.
            Render::FontEffect Effect;
        };

        /// \brief Represents the per-instance data for a glyph.
        struct Layout final
        {
            /// The atlas rectangle the glyph samples, as normalized minimum and maximum edges.
            Array<UInt16, 4> Frame;

            /// The glyph's corner within the text's layout, in subpixel steps.
            Array<SInt16, 2> Offset;

            /// The glyph's extent, in subpixel steps.
            Array<UInt16, 2> Size;

            /// The slot of the run the glyph belongs to, reinterpreted as a float for the vertex stream.
            UInt32           Effect;

            /// The tint applied to the text, as an 8-bit integer color (RGBA).
            IntColor8        Color;
        };

        /// \brief Represents a draw recorded for a glyph, with everything its batch is keyed by.
        struct Command final
        {
            /// The graphics technique to use for rendering the glyph.
            ConstPtr<Graphic::Technique> Technique;

            /// The material to use for rendering the glyph, containing the font atlas and shader parameters.
            ConstPtr<Graphic::Material>  Material;

            /// The generation of the glyph, used for tracking updates and changes.
            UInt16                       Generation;

            /// The input data for the glyph.
            Layout                       Layout;
        };

        /// \brief Represents the slot a glyph indexes its run by.
        struct Slot final
        {
            /// The slot of the run within its palette.
            UInt16 Index;

            /// The palette the run was interned into.
            UInt16 Generation;
        };

        /// \brief Represents a palette of text runs, holding the stream and the runs interned into it.
        struct Palette final
        {
            /// The stream containing the run data.
            Graphic::Stream   Stream;

            /// The runs in the palette, indexed by the slot each glyph carries.
            Sequence<Uniform> Uniforms;
        };

        /// \brief Interns a text run, returning the slot its glyphs index it by.
        ///
        /// \param Effect    The effect every glyph of the run is drawn with.
        /// \param Transform The transform the run is placed by.
        /// \param Size      The size the text is set at, folded into the transform's basis.
        /// \return The slot the run was interned into.
        Slot Intern(ConstRef<Render::FontEffect> Effect, ConstRef<Matrix4x3> Transform, Real32 Size);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Retainer<Graphic::Service>   mService;
        Ref<Render::Collector>       mCollector;
        UInt32                       mKind;
        Retainer<Graphic::Technique> mTechnique;
        Sequence<Command>            mCommands;
        Sequence<Palette>            mPalettes;
    };
}