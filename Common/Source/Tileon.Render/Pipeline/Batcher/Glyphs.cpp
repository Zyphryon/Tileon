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

#include "Glyphs.hpp"

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
        constexpr Real32 kGlyphSubpixel = 512.0f;

        constexpr Real32 Floor = static_cast<Real32>(kMinimum<Type>);
        constexpr Real32 Ceil  = static_cast<Real32>(kMaximum<Type>);

        return static_cast<Type>(Clamp(Round(Value * kGlyphSubpixel), Floor, Ceil));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Glyphs::Glyphs(ConstRetainer<Graphic::Service> Service, Ref<Render::Collector> Collector)
        : mService   { Service },
          mCollector { Collector }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Glyphs::SetTechnique(ConstRetainer<Graphic::Technique> Technique)
    {
        mTechnique = Technique;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Glyphs::Draw(
        ConstRef<Lettering>  Lettering,
        ConstRef<Label>      Label,
        ConstRef<Matrix4x3>  Transform,
        ConstRef<Decoration> Decoration,
        IntColor8            Tint)
    {
        ZY_ASSERT(mTechnique, "A technique must be set before recording text");

        ConstRetainer<Render::Font> Font = Lettering.GetFont();
        const Real32                Size = Lettering.GetSize();

        // The whole run shares one transform and one effect, so they are interned once and every glyph
        // carries only the slot that finds them.
        const GlyphSlot RunData = Intern(Decoration.GetEffect(), Transform, Size);

        ConstRef<Graphic::Technique>      Technique = (* mTechnique);
        const Render::Collector::Priority Priority  = Render::Collector::GetPriority(Technique);
        const Graphic::Object             Pipeline  = Technique.GetHandle();

        const Real32 Order = Transform.GetColumn(2).GetW();

        // The font lays the run out, so what is drawn sits exactly where the same walk measured it.
        Font->Shape(Label.GetContent(), Label.GetSpacing(), [&](ConstRef<Render::Font::Placement> Placement)
        {
            if (Placement.Local.GetWidth() <= 0 || Placement.Local.GetHeight() <= 0)
            {
                return;
            }

            const ConstPtr<Graphic::Material> Material = &* Font->GetMaterial(Placement.Page);

            Ref<GlyphCommand> Command = mGlyphs.Append();
            Command.Generation    = RunData.Generation;
            Command.Material      = Material;
            Command.Technique     = AddressOf(Technique);
            Command.Layout.Effect = static_cast<UInt32>(RunData.Slot);
            Command.Layout.Frame  = Array(
                EncodeUnitCoordinate(Placement.Atlas.GetMinimumX()),
                EncodeUnitCoordinate(Placement.Atlas.GetMinimumY()),
                EncodeUnitCoordinate(Placement.Atlas.GetMaximumX()),
                EncodeUnitCoordinate(Placement.Atlas.GetMaximumY()));
            Command.Layout.Offset    = Array(
                EncodeGlyphCoordinate<SInt16>(Placement.Local.GetX()),
                EncodeGlyphCoordinate<SInt16>(Placement.Local.GetY()));
            Command.Layout.Size      = Array(
                EncodeGlyphCoordinate<UInt16>(Placement.Local.GetWidth()),
                EncodeGlyphCoordinate<UInt16>(Placement.Local.GetHeight()));
            Command.Layout.Color     = Tint;

            const Render::Collector::Object Object(Enum::Cast(Batch::Glyph), mGlyphs.GetSize() - 1);
            mCollector.Push(Object, Priority, Order, RunData.Generation, Pipeline, Material->GetHandle());
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Glyphs::Reset()
    {
        mGlyphs.Clear();
        mPalettes.Clear();
        mTechnique = nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Glyphs::Prepare()
    {
        if (mGlyphs.IsEmpty())
        {
            return;
        }

        for (Ref<GlyphPalette> Palette : mPalettes)
        {
            Graphic::Transient<GlyphRun> Slice = mService->AllocateInFlightUniforms<GlyphRun>(kMaxTextPerBatch);
            Slice.Copy<GlyphRun>(Palette.Runs);

            Palette.Stream = Slice.GetStream();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Glyphs::GlyphSlot Glyphs::Intern(ConstRef<Render::FontEffect> Effect, ConstRef<Matrix4x3> Transform, Real32 Size)
    {
        if (mPalettes.IsEmpty() || mPalettes.GetBack().Runs.GetSize() >= kMaxTextPerBatch)
        {
            mPalettes.Append();
        }

        Ref<GlyphPalette> Palette = mPalettes.GetBack();

        const GlyphSlot Result {
            .Slot       = static_cast<UInt16>(Palette.Runs.GetSize()),
            .Generation = static_cast<UInt16>(mPalettes.GetSize() - 1)
        };

        Ref<GlyphRun> Run = Palette.Runs.Append();
        Run.Transform = Matrix4x3::Pack(Matrix4x3::WithScale(Transform, Vector3(Size)));
        Run.Effect    = Effect;

        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Glyphs::Write(Ref<Render::Encoder> Encoder, ConstSpan<Render::Collector::Command> Commands)
    {
        // Every draw call in this batch shares the same font material and run palette.
        ConstRef<GlyphCommand> First = mGlyphs[Commands.GetFront().Entry.Slot];

        const Graphic::Stream                 Palette = mPalettes[First.Generation].Stream;
        const Graphic::Transient<GlyphLayout> Instances
            = Gather<GlyphLayout, GlyphCommand>(* mService, mGlyphs, Commands);

        const Graphic::Invocation Invocation {
            .Count     = 4,
            .Instances = static_cast<UInt32>(Commands.GetSize())
        };
        Encoder.Draw(* First.Technique, First.Material, Instances.GetStream(), Palette, Invocation);
    }
}