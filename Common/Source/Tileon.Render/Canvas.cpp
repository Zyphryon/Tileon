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

#include "Canvas.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Canvas::Canvas(Ref<Engine::Subsystem::Host> Host)
        : mService { Host.GetService<Graphic::Service>() }
    {
        ConstRetainer<Content::Service> Content = Host.GetService<Content::Service>();
        CreateDefaultResources(Content);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::SetTechnique(Type Type, ConstRetainer<Graphic::Technique> Technique)
    {
        mTechniques[Enum::Cast(Type)] = Technique;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Begin()
    {
        Reset();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawText(ConstRetainer<Render::Font> Font, Real32 Size, Vector2 Spacing, Text Content, ConstRef<Matrix3x2> Transform, Real32 Order, IntColor8 Tint, ConstRef<Render::FontEffect> Effect)
    {
        const GlyphEffect EffectData = InternFontEffect(Effect);

        // The line height in font units.
        const Real32 LineHeight = Font->GetLineHeight(Size) + Spacing.GetY() * Size;

        // Iterate through each character in the text content and generate draw commands for each glyph.
        Real32 CurrentX = 0.0f;
        Real32 CurrentY = 0.0f;
        UInt32 Previous = 0;

        constexpr UInt32 Type      = Enum::Cast(Type::Glyph);
        const     UInt16 Technique = mTechniques[Type]->GetHandle();

        StrIterateUTF8(Content, [&](UInt32 Codepoint)
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
                        Command.Layout.Transform.SetData(Transform, Order, CastBit<Real32>(static_cast<UInt32>(EffectData.Slot)));
                        Command.Layout.Frame     = Glyph->AtlasBounds;
                        Command.Layout.Offset.Set(
                            CurrentX + Glyph->LocalBounds.GetX() * Size,
                            CurrentY + Glyph->LocalBounds.GetY() * Size);
                        Command.Layout.Size      = Vector2(
                            Glyph->LocalBounds.GetWidth()  * Size,
                            Glyph->LocalBounds.GetHeight() * Size);
                        Command.Layout.Color     = Tint;

                        mCollector.Push(
                            Render::Collector::Object(Type, mGlyphs.GetSize() - 1),
                            Render::Collector::Priority::Transparent, Order, EffectData.Generation,
                            Technique,
                            Material->GetHandle());
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

    void Canvas::DrawSprite(ConstRetainer<Graphic::Material> Material, Vector2 Size, ConstRef<Rect> Frame, ConstRef<Matrix3x2> Transform, Real32 Order, IntColor8 Tint)
    {
        Ref<SpriteCommand> Command = mSprites.Append();
        Command.Layout.Transform.SetData(Transform, Order);
        Command.Layout.Frame     = Frame;
        Command.Layout.Size      = Size;
        Command.Layout.Color     = Tint;
        Command.Material         = AddressOf(* Material);
        Command.Technique        = AddressOf(* mTechniques[Enum::Cast(Type::Sprite)]);

        constexpr UInt32                      Type     = Enum::Cast(Type::Sprite);
        constexpr Render::Collector::Priority Priority = Render::Collector::Priority::Opaque;

        const Graphic::Object Surface  = Command.Material->GetHandle();
        const Graphic::Object Pipeline = Command.Technique->GetHandle();
        mCollector.Push(Render::Collector::Object(Type, mSprites.GetSize() - 1), Priority, Order, 0, Pipeline, Surface);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::DrawSpriteAlpha(ConstRetainer<Graphic::Material> Material, Vector2 Size, ConstRef<Rect> Frame, ConstRef<Matrix3x2> Transform, Real32 Order, IntColor8 Tint)
    {
        Ref<SpriteCommand> Command = mSprites.Append();
        Command.Layout.Transform.SetData(Transform, Order);
        Command.Layout.Frame     = Frame;
        Command.Layout.Size      = Size;
        Command.Layout.Color     = Tint;
        Command.Material         = AddressOf(* Material);
        Command.Technique        = AddressOf(* mTechniques[Enum::Cast(Type::SpriteAlpha)]);

        constexpr UInt32                      Type     = Enum::Cast(Type::SpriteAlpha);
        constexpr Render::Collector::Priority Priority = Render::Collector::Priority::Transparent;

        const Graphic::Object Surface  = Command.Material->GetHandle();
        const Graphic::Object Pipeline = Command.Technique->GetHandle();
        mCollector.Push(Render::Collector::Object(Type, mSprites.GetSize() - 1), Priority, Order, 0, Pipeline, Surface);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Flush(Ref<Render::Encoder> Encoder)
    {
        // Prepare any necessary resources or state before processing the collected draw calls.
        Prepare();

        // Iterate through each rendering queue in the collector and encode the batched draw calls into commands.
        mCollector.Poll([&](UInt32 Type, ConstSpan<Render::Collector::Command> Commands)
        {
            switch (static_cast<Canvas::Type>(Type))
            {
            case Type::Sprite:
            case Type::SpriteAlpha:
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

    void Canvas::CreateDefaultResources(ConstRetainer<Content::Service> Content)
    {
        mTechniques[Enum::Cast(Type::Sprite)]      = Content->Load<Graphic::Technique>("Resources://Technique/Geometry/SpriteNonOpaque.vfx");
        mTechniques[Enum::Cast(Type::SpriteAlpha)] = Content->Load<Graphic::Technique>("Resources://Technique/Geometry/SpriteOpaque.vfx");
        mTechniques[Enum::Cast(Type::Glyph)]       = Content->Load<Graphic::Technique>("Resources://Technique/Font/MSDF.vfx");
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Reset()
    {
        // Clear the list of sprites after processing to prepare for the next frame.
        mSprites.Clear();

        // Clear the list of glyphs after processing to prepare for the next frame.
        mGlyphs.Clear();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::Prepare()
    {
        for (Ref<GlyphEffectPalette> Palette : mEffectPalettes)
        {
            Graphic::Transient<Render::FontEffect> Slice = mService->AllocateTransientUniforms<Render::FontEffect>(kMaxEffectsPerBatch);
            Slice.Copy<Render::FontEffect>(Palette.Effects);

            Palette.Stream = Slice.GetStream();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Canvas::GlyphEffect Canvas::InternFontEffect(ConstRef<Render::FontEffect> Effect)
    {
        const UInt64 Key = Hash(Effect);

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

    void Canvas::WriteSprites(Ref<Render::Encoder> Encoder, ConstSpan<Render::Collector::Command> Commands)
    {
        // Every draw call in this batch shares the same technique and material.
        Ref<SpriteCommand> First = mSprites[Commands.GetFront().Entry.Slot];

        // Gather each sprite's per-instance layout into a single transient instance stream.
        Graphic::Transient<SpriteLayout> Instances = mService->AllocateTransientVertices<SpriteLayout>(Commands.GetSize());

        for (UInt32 Element = 0; Element < Commands.GetSize(); ++Element)
        {
            Instances[Element] = mSprites[Commands[Element].Entry.Slot].Layout;
        }

        const Graphic::Invocation Invocation {
            .Count     = 4,
            .Base      = 0,
            .Offset    = 0,
            .Instances = static_cast<UInt32>(Commands.GetSize())
        };
        Encoder.Draw(* First.Technique, First.Material, Instances.GetStream(), Invocation);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Canvas::WriteGlyphs(Ref<Render::Encoder> Encoder, ConstSpan<Render::Collector::Command> Commands)
    {
        // Every draw call in this batch shares the same font material and effect palette.
        Ref<GlyphCommand> First = mGlyphs[Commands.GetFront().Entry.Slot];

        // Gather each glyph's per-instance layout into a single transient instance stream.
        Graphic::Transient<GlyphLayout> Instances = mService->AllocateTransientVertices<GlyphLayout>(Commands.GetSize());

        for (UInt32 Element = 0; Element < Commands.GetSize(); ++Element)
        {
            Instances[Element] = mGlyphs[Commands[Element].Entry.Slot].Layout;
        }

        const Graphic::Stream     Palette  = mEffectPalettes[First.Generation].Stream;
        const Graphic::Invocation Invocation {
            .Count     = 4,
            .Base      = 0,
            .Offset    = 0,
            .Instances = static_cast<UInt32>(Commands.GetSize())
        };
        Encoder.Draw(* mTechniques[Enum::Cast(Type::Glyph)], First.Material, Instances.GetStream(), Palette, Invocation);
    }
}