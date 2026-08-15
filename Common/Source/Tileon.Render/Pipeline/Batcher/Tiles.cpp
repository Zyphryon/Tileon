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

        const Graphic::Object Surface = Glyph.Texture;

        Ptr<TileBatch> Batch = nullptr;

        for (UInt32 Element = 0; Element < mCount; ++Element)
        {
            if (mBatches[Element].Texture == Surface)
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

            // Batches drain in the order they open, so the array the caller draws first is the array drawn first.
            Batch = AddressOf(mBatches[mCount++]);
            Batch->Texture = Surface;
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
        for (UInt32 Element = 0; Element < mCount; ++Element)
        {
            ConstRef<TileBatch> Batch = mBatches[Element];

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
            Encoder.Draw(* mTechnique, ConstSpan(Textures), Instances.GetStream(), Invocation, mVariant);
        }
    }
}