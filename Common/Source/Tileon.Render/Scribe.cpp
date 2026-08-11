// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2021-2026 by Agustin L. Alvarez. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Scribe.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static UInt16 EncodeUnitCoordinate(Real32 Value)
    {
        return static_cast<UInt16>(Clamp(Value, 0.0f, 1.0f) * kMaximum<UInt16> + 0.5f);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Type>
    static Type EncodeGlyphCoordinate(Real32 Value)
    {
        constexpr Real32 kGlyphSubpixel = 8.0f;

        constexpr Real32 Floor = static_cast<Real32>(kMinimum<Type>);
        constexpr Real32 Ceil  = static_cast<Real32>(kMaximum<Type>);

        return static_cast<Type>(Clamp(Round(Value * kGlyphSubpixel), Floor, Ceil));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scribe::Scribe(Ref<Engine::Subsystem::Host> Host)
        : mService    { Host.GetService<Graphic::Service>() },
          mTilesCount { 0 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scribe::SetTechnique(ConstRetainer<Graphic::Technique> Technique)
    {
        mTechnique = Technique;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scribe::Begin()
    {
        Reset();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scribe::DrawTile(ConstRef<Tileset::Glyph> Glyph, IntVector2 Phase, IntVector2 Position, IntVector2 Size, UInt8 Layer)
    {
        ZY_ASSERT(mTechnique, "A technique must be set before recording a tile");

        const ConstPtr<Graphic::Technique> Pipeline = AddressOf(* mTechnique);
        const Graphic::Object              Surface  = Glyph.Texture;

        Ptr<TileBatch> Batch = nullptr;

        for (UInt32 Element = 0; Element < mTilesCount; ++Element)
        {
            if (mTilesBatches[Element].Technique == Pipeline && mTilesBatches[Element].Texture == Surface)
            {
                Batch = AddressOf(mTilesBatches[Element]);
                break;
            }
        }

        if (!Batch)
        {
            // Batches retired by an earlier frame are reused, so their instance storage survives the frame.
            if (mTilesCount == mTilesBatches.GetSize())
            {
                mTilesBatches.Append();
            }

            // Batches drain in the order they open, so the layer the caller draws first is the layer drawn first.
            Batch = AddressOf(mTilesBatches[mTilesCount++]);
            Batch->Technique = Pipeline;
            Batch->Texture   = Surface;
        }

        Ref<TileLayout> Layout = Batch->Layouts.Append();
        Layout.Position = Array(static_cast<SInt16>(Position.GetX()), static_cast<SInt16>(Position.GetY()));
        Layout.Metrics  = Array(
            static_cast<UInt8>(Size.GetX()),
            static_cast<UInt8>(Size.GetY()),
            Layer,
            static_cast<UInt8>(0));
        Layout.Lattice  = Array(
            static_cast<UInt8>(Max<SInt32>(Glyph.Period.GetX(), 1)),
            static_cast<UInt8>(Max<SInt32>(Glyph.Period.GetY(), 1)),
            static_cast<UInt8>(Phase.GetX()),
            static_cast<UInt8>(Phase.GetY()));
        Layout.Slice    = Glyph.Slice;
        Layout.Color    = Glyph.Tint;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scribe::DrawSprite(ConstRef<Appearance> Appearance, Vector2 Size, ConstRef<Matrix4x3> Transform, IntColor8 Tint)
    {
        ZY_ASSERT(mTechnique, "A technique must be set before recording a sprite");

        ConstRef<Graphic::Technique> Technique = (* mTechnique);

        Ref<SpriteCommand> Command = mSprites.Append();
        Command.Layout.Transform.SetData(Transform);
        Command.Layout.Frame  = Appearance.GetSource();
        Command.Layout.Size   = Size;
        Command.Layout.Color  = Tint;
        Command.Material      = AddressOf(* Appearance.GetMaterial());
        Command.Technique     = AddressOf(Technique);

        const Real32                    Order = Transform.GetColumn(2).GetW();
        const Render::Collector::Object Object(Enum::Cast(Type::Sprite), mSprites.GetSize() - 1);
        mCollector.Push(Object, GetPriority(Technique), Order, 0, Technique.GetHandle(), Command.Material->GetHandle());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scribe::DrawText(ConstRef<Lettering> Lettering, ConstRef<Label> Label, ConstRef<Matrix4x3> Transform, ConstRef<Decoration> Decoration, IntColor8 Tint)
    {
        ZY_ASSERT(mTechnique, "A technique must be set before recording text");

        const GlyphEffect EffectData = InternFontEffect(Decoration.GetEffect());

        ConstRetainer<Render::Font> Font = Lettering.GetFont();
        const Real32                Size = Lettering.GetSize();

        // The line height in font units.
        const Real32 LineHeight = Font->GetLineHeight(Size) + Label.GetSpacing().GetY() * Size;

        // Iterate through each character in the text content and generate draw commands for each glyph.
        Real32 CurrentX = 0.0f;
        Real32 CurrentY = 0.0f;
        UInt32 Previous = 0;

        ConstRef<Graphic::Technique>      Technique = (* mTechnique);
        const Render::Collector::Priority Priority  = GetPriority(Technique);
        const Graphic::Object             Pipeline  = Technique.GetHandle();

        const Real32 Order = Transform.GetColumn(2).GetW();

        StrIterateUTF8(Label.GetContent(), [&](UInt32 Codepoint)
        {
            switch (Codepoint)
            {
            case '\r':
                CurrentX = 0.0f;
                break;
            case '\n':
                CurrentY -= LineHeight;
                break;
            default:
                const ConstPtr<Render::Font::Glyph> Glyph = Font->GetGlyph(Codepoint);

                if (Glyph)
                {
                    if (Glyph->LocalBounds.GetWidth() > 0 && Glyph->LocalBounds.GetHeight() > 0)
                    {
                        const ConstPtr<Graphic::Material> Material = &* Font->GetMaterial(Glyph->Page);

                        Ref<GlyphCommand> Command = mGlyphs.Append();
                        Command.Generation       = EffectData.Generation;
                        Command.Material         = Material;
                        Command.Technique        = AddressOf(Technique);
                        Command.Layout.Transform.SetData(Transform);
                        Command.Layout.Effect    = CastBit<Real32>(static_cast<UInt32>(EffectData.Slot));
                        Command.Layout.Frame     = Array(
                            EncodeUnitCoordinate(Glyph->AtlasBounds.GetMinimumX()),
                            EncodeUnitCoordinate(Glyph->AtlasBounds.GetMinimumY()),
                            EncodeUnitCoordinate(Glyph->AtlasBounds.GetMaximumX()),
                            EncodeUnitCoordinate(Glyph->AtlasBounds.GetMaximumY()));
                        Command.Layout.Offset    = Array(
                            EncodeGlyphCoordinate<SInt16>(CurrentX + Glyph->LocalBounds.GetX() * Size),
                            EncodeGlyphCoordinate<SInt16>(CurrentY + Glyph->LocalBounds.GetY() * Size));
                        Command.Layout.Size      = Array(
                            EncodeGlyphCoordinate<UInt16>(Glyph->LocalBounds.GetWidth()  * Size),
                            EncodeGlyphCoordinate<UInt16>(Glyph->LocalBounds.GetHeight() * Size));
                        Command.Layout.Color     = Tint;

                        // Each glyph enters the queue under its own slot, while the run shares one order so
                        // that it stays together and drains as a single batch.
                        const Render::Collector::Object Object(Enum::Cast(Type::Glyph), mGlyphs.GetSize() - 1);
                        mCollector.Push(Object, Priority, Order, EffectData.Generation, Pipeline, Material->GetHandle());
                    }
                    CurrentX += (Font->GetKerning(Previous, Codepoint) + Glyph->Advance) * Size;
                }
                break;
            }
            Previous = Codepoint;
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scribe::Flush(Ref<Render::Encoder> Encoder)
    {
        // Prepare any necessary resources or state before processing the collected draw calls.
        Prepare();

        // Tiles never enter the collector, so they are drained first, laying the ground the rest draws over.
        WriteTiles(Encoder);

        // Iterate through each rendering queue in the collector and encode the batched draw calls into commands.
        mCollector.Poll([&](UInt32 Type, ConstSpan<Render::Collector::Command> Commands)
        {
            switch (static_cast<Scribe::Type>(Type))
            {
            case Type::Sprite:
                WriteSprites(Encoder, Commands);
                break;
            case Type::Glyph:
                WriteGlyphs(Encoder, Commands);
                break;
            }
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scribe::Reset()
    {
        // Dropping the technique keeps a pass from inheriting the pipeline the pass before it drew with.
        mTechnique = nullptr;

        // Retire the tile batches while keeping their instance storage, so the next frame draws into it again.
        for (UInt32 Element = 0; Element < mTilesCount; ++Element)
        {
            mTilesBatches[Element].Layouts.Clear();
        }
        mTilesCount = 0;

        mSprites.Clear();
        mGlyphs.Clear();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scribe::Prepare()
    {
        // Palettes outlive the pass that interned them, so a pass drawing no text would re-upload them for nothing.
        if (mGlyphs.IsEmpty())
        {
            return;
        }

        for (Ref<GlyphEffectPalette> Palette : mEffectPalettes)
        {
            Graphic::Transient<Render::FontEffect> Slice = mService->AllocateInFlightUniforms<Render::FontEffect>(kMaxEffectsPerBatch);
            Slice.Copy<Render::FontEffect>(Palette.Effects);

            Palette.Stream = Slice.GetStream();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scribe::GlyphEffect Scribe::InternFontEffect(ConstRef<Render::FontEffect> Effect)
    {
        const Digest Key(Hash(Effect));

        if (const ConstPtr<GlyphEffect> Existing = mEffectLookup.Find(Key))
        {
            return (* Existing);
        }

        if (mEffectPalettes.IsEmpty() || mEffectPalettes.GetBack().Effects.GetSize() >= kMaxEffectsPerBatch)
        {
            mEffectPalettes.Append();
        }

        Ref<GlyphEffectPalette> Palette = mEffectPalettes.GetBack();

        const GlyphEffect Result {
            .Slot       = static_cast<UInt16>(Palette.Effects.GetSize()),
            .Generation = static_cast<UInt16>(mEffectPalettes.GetSize() - 1)
        };

        Palette.Effects.Append(Effect);
        mEffectLookup.Assign(Key, Result);
        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scribe::WriteTiles(Ref<Render::Encoder> Encoder)
    {
        for (UInt32 Element = 0; Element < mTilesCount; ++Element)
        {
            ConstRef<TileBatch> Batch = mTilesBatches[Element];

            if (Batch.Layouts.IsEmpty())
            {
                continue;
            }

            Graphic::Transient<TileLayout> Instances = mService->AllocateInFlightVertices<TileLayout>(Batch.Layouts.GetSize());
            Instances.Copy<TileLayout>(Batch.Layouts);

            const Graphic::Invocation Invocation {
                .Count     = 4,
                .Instances = static_cast<UInt32>(Batch.Layouts.GetSize())
            };
            const Graphic::Object Textures[] = { Batch.Texture };
            Encoder.Draw(* Batch.Technique, ConstSpan(Textures), Instances.GetStream(), Invocation);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scribe::WriteSprites(Ref<Render::Encoder> Encoder, ConstSpan<Render::Collector::Command> Commands)
    {
        // Every draw call in this batch shares the same technique and material.
        ConstRef<SpriteCommand> First = mSprites[Commands.GetFront().Entry.Slot];

        const Graphic::Transient<SpriteLayout> Instances = Gather<SpriteLayout>(mSprites, Commands);

        const Graphic::Invocation Invocation {
            .Count     = 4,
            .Instances = static_cast<UInt32>(Commands.GetSize())
        };
        Encoder.Draw(* First.Technique, First.Material, Instances.GetStream(), Invocation);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scribe::WriteGlyphs(Ref<Render::Encoder> Encoder, ConstSpan<Render::Collector::Command> Commands)
    {
        // Every draw call in this batch shares the same font material and effect palette.
        ConstRef<GlyphCommand> First = mGlyphs[Commands.GetFront().Entry.Slot];

        const Graphic::Transient<GlyphLayout> Instances = Gather<GlyphLayout>(mGlyphs, Commands);
        const Graphic::Stream                 Palette   = mEffectPalettes[First.Generation].Stream;

        const Graphic::Invocation Invocation {
            .Count     = 4,
            .Instances = static_cast<UInt32>(Commands.GetSize())
        };
        Encoder.Draw(* First.Technique, First.Material, Instances.GetStream(), Palette, Invocation);
    }
}