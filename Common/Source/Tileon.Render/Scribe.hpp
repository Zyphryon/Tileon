// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Tileset.hpp"
#include "Component/Appearance.hpp"
#include "Component/Decoration.hpp"
#include "Component/Label.hpp"
#include "Component/Lettering.hpp"
#include "Zyphryon.Content/Service.hpp"
#include "Zyphryon.Render/Collector.hpp"
#include "Zyphryon.Render/Encoder.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Records the tiles, sprites and text of a frame, and drains them as batched draw calls.
    class Scribe final  // TODO: Needs Refactoring
    {
    public:

        /// \brief Enumeration of the kinds of draw the collector dispatches back when drained.
        ///
        /// \remark Tiles are absent because they never enter the collector, being batched on their own.
        enum class Type : UInt8
        {
            Sprite,         ///< A sprite for drawing textured quads.
            Glyph,          ///< A text glyph for drawing characters from a font atlas.
        };

    public:

        /// \brief Constructs a scribe with the specified service host.
        ///
        /// \param Host The service host to use for rendering operations.
        Scribe(Ref<Engine::Subsystem::Host> Host);

        /// \brief Sets the technique subsequent draw calls are recorded under.
        ///
        /// \param Technique The technique to set for subsequent draw calls.
        void SetTechnique(ConstRetainer<Graphic::Technique> Technique);

        /// \brief Begins a new pass, dropping the technique so each pass states the one it draws with.
        void Begin();

        /// \brief Issues a draw command for drawing a tile lying flat on the ground plane.
        ///
        /// \param Glyph    The glyph the tile shows.
        /// \param Phase    Where the tile's origin falls within the glyph's period, from \ref Mosaic::GetPhase.
        /// \param Position The position of the tile's origin, in whole tiles relative to the camera.
        /// \param Size     The span the tile covers on the ground, in whole tiles.
        /// \param Layer    The layer the tile stacks on, where higher layers are drawn in front.
        void DrawTile(ConstRef<Tileset::Glyph> Glyph, IntVector2 Phase, IntVector2 Position, IntVector2 Size, UInt8 Layer);

        /// \brief Issues a draw command for drawing a sprite with the specified parameters.
        ///
        /// \param Appearance The appearance supplying the material the sprite samples and the region it shows.
        /// \param Size       The size the sprite covers, in world units, before the transform is applied.
        /// \param Transform  The transformation matrix to apply to the sprite for positioning, scaling, and rotation.
        /// \param Tint       The tint color to apply to the sprite.
        void DrawSprite(ConstRef<Appearance> Appearance, Vector2 Size, ConstRef<Matrix4x3> Transform, IntColor8 Tint);

        /// \brief Issues a draw command for drawing text with the specified parameters.
        ///
        /// \param Lettering  The lettering supplying the font the glyphs come from and the size they are set at.
        /// \param Label      The label supplying the content to draw and the spacing it is laid out with.
        /// \param Transform  The transformation matrix to apply to the text for positioning, scaling, and rotation.
        /// \param Decoration The decoration supplying the effect the glyphs are drawn with.
        /// \param Tint       The tint color to apply to the glyphs.
        void DrawText(ConstRef<Lettering> Lettering, ConstRef<Label> Label, ConstRef<Matrix4x3> Transform, ConstRef<Decoration> Decoration, IntColor8 Tint);

        /// \brief Drains the draw calls recorded since \ref Begin, encoding them as batched draw commands.
        ///
        /// \param Encoder The encoder that builds and binds the resulting draw commands.
        void Flush(Ref<Render::Encoder> Encoder);

    private:

        /// \brief Maximum number of text effects that can be batched together in a single draw call.
        static constexpr UInt32 kMaxEffectsPerBatch = 64;

        /// \brief Defines a structure carrying a 4x3 transformation matrix in the layout the shaders consume.
        struct Matrix4x3Packed final
        {
            /// \brief The X row of the basis.
            Vector3 Basis0;

            /// \brief The translation along X.
            Real32  OffsetX;

            /// \brief The Y row of the basis.
            Vector3 Basis1;

            /// \brief The translation along Y.
            Real32  OffsetY;

            /// \brief The Z row of the basis.
            Vector3 Basis2;

            /// \brief The translation along Z.
            Real32  OffsetZ;

            /// \brief Sets the values of the matrix from a 4x3 transformation matrix.
            ///
            /// \param Matrix The 4x3 transformation matrix to convert into the packed format.
            ZY_INLINE void SetData(ConstRef<Matrix4x3> Matrix)
            {
                const Vector4 Column0 = Matrix.GetColumn(0);
                const Vector4 Column1 = Matrix.GetColumn(1);
                const Vector4 Column2 = Matrix.GetColumn(2);

                Basis0  = Column0.GetXYZ();
                OffsetX = Column0.GetW();
                Basis1  = Column1.GetXYZ();
                OffsetY = Column1.GetW();
                Basis2  = Column2.GetXYZ();
                OffsetZ = Column2.GetW();
            }
        };

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

        /// \brief Gathers every tile drawn with one technique and sampling one material into a single instanced draw.
        struct TileBatch final
        {
            /// The technique every tile of the batch is drawn with.
            ConstPtr<Graphic::Technique> Technique;

            /// The array texture every tile of the batch samples, each from its own slice.
            Graphic::Object              Texture;

            /// The per-instance input data of the batch's tiles.
            Sequence<TileLayout>         Layouts;
        };

        /// \brief Defines a structure representing the input data for drawing a sprite in the GPU.
        struct SpriteLayout final
        {
            /// The transformation matrix to apply to the sprite for positioning, scaling, and rotation.
            Matrix4x3Packed Transform;

            /// The source rectangle within the sprite's texture, defining the portion of the texture to use.
            Rect            Frame;

            /// Additional data for the sprite, such as size stored as a 2D vector.
            Vector2         Size;

            /// Color tint to apply to the sprite, represented as an 8-bit integer color (RGBA).
            IntColor8       Color;
        };

        /// \brief Defines a draw command for a sprite, containing its input data.
        struct SpriteCommand final
        {
            /// The graphics technique to use for rendering the sprite.
            ConstPtr<Graphic::Technique> Technique;

            /// The material to use for rendering the sprite, containing shader parameters and resources.
            ConstPtr<Graphic::Material>  Material;

            /// The input data for the sprite.
            SpriteLayout                 Layout;
        };

        /// \brief Defines a structure representing the input data for drawing a glyph in the GPU.
        struct GlyphLayout final
        {
            /// The transformation matrix to apply to the text for positioning, scaling, and rotation.
            Matrix4x3Packed Transform;

            /// The atlas rectangle the glyph samples, as normalized minimum and maximum edges.
            Array<UInt16, 4> Frame;

            /// The glyph's corner within the text's layout, in subpixel steps.
            Array<SInt16, 2> Offset;

            /// The glyph's extent, in subpixel steps.
            Array<UInt16, 2> Size;

            /// The slot of the text effect applied to the glyph, reinterpreted as a float for the vertex stream.
            Real32           Effect;

            /// Color tint to apply to the text, represented as an 8-bit integer color (RGBA).
            IntColor8        Color;
        };

        /// \brief Structure representing a draw command for a glyph, containing its input data.
        struct GlyphCommand final
        {
            /// The graphics technique to use for rendering the glyph.
            ConstPtr<Graphic::Technique> Technique;

            /// The material to use for rendering the glyph, containing the font atlas and shader parameters.
            ConstPtr<Graphic::Material>  Material;

            /// The generation of the glyph, used for tracking updates and changes.
            UInt16                       Generation;

            /// The input data for the glyph.
            GlyphLayout                  Layout;
        };

        /// \brief Structure representing a text effect applied to a glyph.
        struct GlyphEffect
        {
            /// The slot index of the text effect, used for managing multiple effects.
            UInt16 Slot;

            /// The generation of the text effect, used for tracking updates and changes.
            UInt16 Generation;
        };

        /// \brief Structure representing a palette of text effects, containing the stream, lookup table, and sequence of effects.
        struct GlyphEffectPalette
        {
            /// The stream containing the text effect data.
            Graphic::Stream                   Stream;

            /// The lookup table mapping text effects to their corresponding indices.
            Table<Render::FontEffect, UInt32> Lookup;

            /// The sequence of text effects in the palette.
            Sequence<Render::FontEffect>      Effects;
        };

        /// \brief Resolves the queue a technique's output belongs to from the blend state it declares.
        ///
        /// \param Technique The technique whose fixed-function blend state decides the queue.
        /// \return The queue the technique's draws are collected into.
        ZY_INLINE Render::Collector::Priority GetPriority(ConstRef<Graphic::Technique> Technique)
        {
            ConstRef<Graphic::States> States = Technique.GetDescription().States;

            const Bool Opaque = (States.BlendSrcColor == Graphic::BlendFactor::One
                              && States.BlendDstColor == Graphic::BlendFactor::Zero);
            return Opaque ? Render::Collector::Priority::Opaque : Render::Collector::Priority::Transparent;
        }

        /// \brief Gathers the per-instance input of a batch's draw calls into a transient instance stream.
        ///
        /// \param Source   The commands the batch indexes into.
        /// \param Commands The span of draw calls forming the batch.
        /// \return The stream holding the batch's per-instance input, in the batch's own order.
        template<typename Layout, typename Command>
        ZY_INLINE Graphic::Transient<Layout> Gather(ConstRef<Sequence<Command>> Source, ConstSpan<Render::Collector::Command> Commands)
        {
            Graphic::Transient<Layout> Instances = mService->AllocateInFlightVertices<Layout>(Commands.GetSize());

            for (UInt32 Element = 0; Element < Commands.GetSize(); ++Element)
            {
                Instances[Element] = Source[Commands[Element].Entry.Slot].Layout;
            }
            return Instances;
        }

        /// \brief Resets the renderer's internal state, clearing any pending commands and bindings.
        void Reset();

        /// \brief Uploads the effect palettes the pass's glyphs index into.
        void Prepare();

        /// \brief Interns a text effect, returning a glyph effect that can be used for rendering.
        ///
        /// \param Effect The text effect to intern.
        /// \return The interned glyph effect.
        GlyphEffect InternFontEffect(ConstRef<Render::FontEffect> Effect);

        /// \brief Writes each batch of tiles as a single instanced command through the encoder.
        ///
        /// \param Encoder The encoder that builds the resulting draw commands.
        void WriteTiles(Ref<Render::Encoder> Encoder);

        /// \brief Writes a batch of sprite draw calls as a single instanced command through the encoder.
        ///
        /// \param Encoder  The encoder that builds the resulting draw command.
        /// \param Commands The span of sprite draw calls to be processed and encoded for rendering.
        void WriteSprites(Ref<Render::Encoder> Encoder, ConstSpan<Render::Collector::Command> Commands);

        /// \brief Writes a batch of glyph draw calls as a single instanced command through the encoder.
        ///
        /// \param Encoder  The encoder that builds the resulting draw command.
        /// \param Commands The span of glyph draw calls to be processed and encoded for rendering.
        void WriteGlyphs(Ref<Render::Encoder> Encoder, ConstSpan<Render::Collector::Command> Commands);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Retainer<Graphic::Service>     mService;
        Render::Collector              mCollector;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Retainer<Graphic::Technique>   mTechnique;
        Sequence<SpriteCommand>        mSprites;
        Sequence<GlyphCommand>         mGlyphs;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Sequence<TileBatch>            mTilesBatches;
        UInt32                         mTilesCount;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Sequence<GlyphEffectPalette>   mEffectPalettes;
        Table<Digest, GlyphEffect>     mEffectLookup;
    };
}