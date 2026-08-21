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

#include "Glyph.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Batch
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

    Glyph::Glyph(ConstRetainer<Graphic::Service> Service, Ref<Render::Collector> Collector, UInt32 Kind)
        : mService   { Service },
          mCollector { Collector },
          mKind      { Kind }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Glyph::SetTechnique(ConstRetainer<Graphic::Technique> Technique)
    {
        mTechnique = Technique;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Glyph::Draw(ConstRef<Typeface> Face, ConstRef<Label> Label, ConstRef<Matrix4x3> Transform, ConstRef<Contour> Effect, IntColor8 Tint)
    {
        ZY_ASSERT(mTechnique, "A technique must be set before recording text");

        ConstRetainer<Render::Font> Font = Face.GetFont();
        const Real32                Size = Face.GetSize();

        const Slot Data = Intern(Effect.GetEffect(), Transform, Size);

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

            Ref<Command> Entry = mCommands.Append();
            Entry.Generation    = Data.Generation;
            Entry.Material      = Material;
            Entry.Technique     = AddressOf(Technique);
            Entry.Layout.Effect = static_cast<UInt32>(Data.Index);
            Entry.Layout.Frame  = Array(
                EncodeUnitCoordinate(Placement.Atlas.GetMinimumX()),
                EncodeUnitCoordinate(Placement.Atlas.GetMinimumY()),
                EncodeUnitCoordinate(Placement.Atlas.GetMaximumX()),
                EncodeUnitCoordinate(Placement.Atlas.GetMaximumY()));
            Entry.Layout.Offset = Array(
                EncodeGlyphCoordinate<SInt16>(Placement.Local.GetX()),
                EncodeGlyphCoordinate<SInt16>(Placement.Local.GetY()));
            Entry.Layout.Size   = Array(
                EncodeGlyphCoordinate<UInt16>(Placement.Local.GetWidth()),
                EncodeGlyphCoordinate<UInt16>(Placement.Local.GetHeight()));
            Entry.Layout.Color  = Tint;

            const Render::Collector::Object Object(mKind, mCommands.GetSize() - 1);
            mCollector.Push(Object, Priority, Order, Data.Generation, Pipeline, Material->GetHandle());
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Glyph::Reset()
    {
        mCommands.Clear();
        mPalettes.Clear();
        mTechnique = nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Glyph::Prepare()
    {
        if (mCommands.IsEmpty())
        {
            return;
        }

        for (Ref<Palette> Slot : mPalettes)
        {
            Graphic::Transient<Uniform> Slice = mService->AllocateInFlightUniforms<Uniform>(kMaxTextPerBatch);
            Slice.Copy<Uniform>(Slot.Uniforms);

            Slot.Stream = Slice.GetStream();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Glyph::Write(Ref<Render::Encoder> Encoder, ConstSpan<Render::Collector::Command> Commands)
    {
        // Every draw call in this batch shares the same font material and run palette.
        ConstRef<Command> First = mCommands[Commands.GetFront().Entry.Slot];

        const Graphic::Stream      Uniforms  = mPalettes[First.Generation].Stream;
        Graphic::Transient<Layout> Instances = mService->AllocateInFlightVertices<Layout>(Commands.GetSize());

        for (UInt32 Element = 0; Element < Commands.GetSize(); ++Element)
        {
            Instances[Element] = mCommands[Commands[Element].Entry.Slot].Layout;
        }

        const Graphic::Invocation Invocation {
            .Count     = 4,
            .Instances = static_cast<UInt32>(Commands.GetSize())
        };
        Encoder.Draw(* First.Technique, First.Material, Instances.GetStream(), Uniforms, Invocation);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Glyph::Slot Glyph::Intern(ConstRef<Render::FontEffect> Effect, ConstRef<Matrix4x3> Transform, Real32 Size)
    {
        if (mPalettes.IsEmpty() || mPalettes.GetBack().Uniforms.GetSize() >= kMaxTextPerBatch)
        {
            mPalettes.Append();
        }

        Ref<Palette> Bucket = mPalettes.GetBack();

        const Slot Result {
            .Index      = static_cast<UInt16>(Bucket.Uniforms.GetSize()),
            .Generation = static_cast<UInt16>(mPalettes.GetSize() - 1)
        };

        Ref<Uniform> Entry = Bucket.Uniforms.Append();
        Entry.Transform = Matrix4x3::Pack(Matrix4x3::WithScale(Transform, Vector3(Size)));
        Entry.Effect    = Effect;

        return Result;
    }
}