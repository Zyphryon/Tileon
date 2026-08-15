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

#include "Tiles.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Tiles::Tiles(ConstRetainer<Graphic::Service> Service)
        : mService { Service },
          mVariant { 0 },
          mCount   { 0 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tiles::SetTechnique(ConstRetainer<Graphic::Technique> Technique, Graphic::Technique::Key Variant)
    {
        mTechnique = Technique;
        mVariant   = Variant;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tiles::Draw(ConstRef<Tileset::Glyph> Glyph, IntVector2 Phase, IntVector2 Position, IntVector2 Size, UInt8 Layer)
    {
        ZY_ASSERT(mTechnique, "A technique must be set before recording a tile");

        // Both arrays follow the sheet, so the albedo alone says which batch a tile belongs to.
        const Graphic::Object Surface = Glyph.GetTexture(Motif::Source::Albedo);

        Ptr<TileBatch> Batch = nullptr;

        for (UInt32 Element = 0; Element < mCount; ++Element)
        {
            if (mBatches[Element].Textures[Enum::Cast(Motif::Source::Albedo)] == Surface)
            {
                Batch = AddressOf(mBatches[Element]);
                break;
            }
        }

        if (!Batch)
        {
            // Batches retired by an earlier frame are reused, so their instance storage survives the frame.
            if (mCount == mBatches.GetSize())
            {
                mBatches.Append();
            }

            // Batches drain in the order they open, so the layer the caller draws first is the layer drawn first.
            Batch = AddressOf(mBatches[mCount++]);
            Batch->Textures = Glyph.Textures;
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

    void Tiles::Reset()
    {
        for (UInt32 Element = 0; Element < mCount; ++Element)
        {
            mBatches[Element].Layouts.Clear();
        }
        mCount     = 0;
        mTechnique = nullptr;
        mVariant   = 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tiles::Flush(Ref<Render::Encoder> Encoder)
    {
        const Graphic::Technique::Key Lit = mTechnique->Resolve("Lit");

        for (UInt32 Element = 0; Element < mCount; ++Element)
        {
            ConstRef<TileBatch> Batch = mBatches[Element];

            if (Batch.Layouts.IsEmpty())
            {
                continue;
            }

            Graphic::Transient<TileLayout> Instances
                = mService->AllocateInFlightVertices<TileLayout>(Batch.Layouts.GetSize());
            Instances.Copy<TileLayout>(Batch.Layouts);

            const Graphic::Invocation Invocation {
                .Count     = 4,
                .Instances = static_cast<UInt32>(Batch.Layouts.GetSize())
            };

            const Graphic::Technique::Key Variant
                = mVariant | (Batch.Textures[Enum::Cast(Motif::Source::Normal)] ? Lit : 0);
            Encoder.Draw(* mTechnique, Batch.Textures, Instances.GetStream(), Invocation, Variant);
        }
    }
}