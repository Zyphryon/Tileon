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

#include "Splatter.hpp"
#include "Tileon.Render/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Splatter::Splatter(ConstRetainer<Graphic::Service> Service, Ref<Splatset> Splatset, Real32 Density)
        : mService    { Service },
          mSplatset   { Splatset },
          mDensity    { Density },
          mRebuilding { false }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Splatter::Record(ConstRef<Region> Region, Ref<Splatmap> Splat, Bool Visible)
    {
        Ref<Entry> Item = mRegions.Append();
        Item.Component = AddressOf(Region);
        Item.Splat     = AddressOf(Splat);
        Item.Drawable  = Visible && Splat.IsPainted();

        // Only a region that will actually write a page needs the neighbour bookkeeping.
        mRebuilding |= (Item.Drawable && Splat.IsInvalidated());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Splatter::Draw(Ref<Render::Encoder> Encoder, ConstRetainer<Graphic::Technique> Technique, IntVector3 Origin)
    {
        ConstRetainer<Graphic::Material> Material = mSplatset.GetMaterial();

        if (Material->HasCompleted())
        {
            // Nothing but a rebuild ever reads a neighbour, so a frame that changes no page pays for none of
            // the bookkeeping below.
            if (mRebuilding)
            {
                Spread();
            }

            for (ConstRef<Entry> Item : mRegions)
            {
                if (!Item.Drawable)
                {
                    continue;
                }

                Ref<Splatmap> Splat = * Item.Splat;

                if (Splat.GetPage() == Splatmap::kUnassigned)
                {
                    if (mAvailable.IsEmpty())
                    {
                        Allocate();
                    }

                    Splat.Assign(mAvailable.GetBack());
                    mAvailable.RemoveLast();
                }

                if (Splat.IsInvalidated())
                {
                    Upload(* Item.Component, Splat);
                }
            }

            // A terrain covers as much ground as its art measures, before its own tiling stretches it.
            ConstRetainer<Graphic::Image> Albedo = Material->GetImage(GetTextureID(Texture::Albedo));
            const Real32 Scale = (Albedo && Albedo->GetWidth() > 0) ? mDensity / Albedo->GetWidth() : 1.0f;

            // One draw binds one array, so the ground goes down a page store at a time.
            for (UInt32 Bank = 0; Bank < mTextures.GetSize(); ++Bank)
            {
                Graphic::Transient<Layout> Instances = mService->AllocateInFlightVertices<Layout>(mRegions.GetSize());

                UInt32 Drawn = 0;

                for (ConstRef<Entry> Item : mRegions)
                {
                    ConstRef<Splatmap> Splat = * Item.Splat;
                    const UInt16       Page  = Splat.GetPage();

                    if (!Item.Drawable || Page == Splatmap::kUnassigned || Page / kPage != Bank)
                    {
                        continue;
                    }

                    const SInt32 RegionX = Item.Component->GetX() * Region::kUnitsPerX;
                    const SInt32 RegionY = Item.Component->GetY() * Region::kUnitsPerY;

                    Ref<Layout> Instance = Instances[Drawn++];
                    Instance.Origin  = IntVector2(RegionX - Origin.GetX(), RegionY - Origin.GetZ());
                    Instance.Weights = Page % kPage;

                    for (UInt8 Slot = 0; Slot < Splatmap::kSlots; ++Slot)
                    {
                        const UInt16 Slice = Splat.GetSplat(Slot);

                        ConstRef<Splatset::Terrain> Terrain = mSplatset.GetTerrain(Slice);

                        const Real32 Repeat = Scale * Terrain.Tiling;
                        const Real64 SweepX = static_cast<Real64>(RegionX) * Repeat;
                        const Real64 SweepY = static_cast<Real64>(RegionY) * Repeat;

                        Instance.Palette[Slot]       = Slice;
                        Instance.Mapping[Slot]       = Repeat;
                        Instance.Phase[Slot * 2 + 0] = static_cast<Real32>(SweepX - static_cast<SInt64>(SweepX));
                        Instance.Phase[Slot * 2 + 1] = static_cast<Real32>(SweepY - static_cast<SInt64>(SweepY));
                        Instance.Tint[Slot]          = Terrain.Tint;
                        Instance.Feather[Slot]       = Terrain.Feather;
                    }
                }

                if (Drawn > 0)
                {
                    const Graphic::Invocation Invocation {
                        .Count     = 4,
                        .Base      = 0,
                        .Offset    = 0,
                        .Instances = Drawn
                    };

                    Render::Encoder::Binder Binder = Encoder.Begin(* Technique);
                    Binder.Apply(* Material);
                    Binder.SetImage("Weight"_Hash, mTextures[Bank]);
                    Binder.Draw(Instances.GetStream(), Invocation);
                }
            }
        }

        mIndices.Clear();
        mRegions.Clear();
        mRebuilding = false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Splatter::Release(Ref<Splatmap> Splat)
    {
        if (Splat.GetPage() != Splatmap::kUnassigned)
        {
            mAvailable.Append(Splat.GetPage());

            Splat.Release();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Splatter::Allocate()
    {
        const UInt16 Base = static_cast<UInt16>(mTextures.GetSize() * kPage);

        mTextures.Append(mService->CreateTexture(
            Graphic::TextureLayout::Texture2DArray,
            Graphic::TextureFormat::RGBA8UIntNorm,
            Graphic::Storage::Stream,
            Graphic::Usage::Sample,
            kMapSize,
            kMapSize,
            kPage,
            1,
            Graphic::Multisample::X1,
            Blob()));

        for (UInt16 Page = kPage; Page > 0; --Page)
        {
            mAvailable.Append(Base + Page - 1);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Splatter::Spread()
    {
        for (UInt32 Slot = 0; Slot < mRegions.GetSize(); ++Slot)
        {
            ConstRef<Entry> Item = mRegions[Slot];

            mIndices.Assign(GetKey(Item.Component->GetX(), Item.Component->GetY()), Slot);
        }

        // A region's edge is written into the pages around it as much as into its own, so a stroke that
        // reached a border leaves its neighbours stale too.
        for (ConstRef<Entry> Item : mRegions)
        {
            if (!Item.Splat->IsSpreading())
            {
                continue;
            }

            Item.Splat->Settle();

            const SInt32 RegionX = Item.Component->GetX();
            const SInt32 RegionY = Item.Component->GetY();

            for (SInt32 OffsetY = -1; OffsetY <= 1; ++OffsetY)
            {
                for (SInt32 OffsetX = -1; OffsetX <= 1; ++OffsetX)
                {
                    if (const ConstPtr<UInt32> Slot = mIndices.Find(GetKey(RegionX + OffsetX, RegionY + OffsetY)))
                    {
                        mRegions[* Slot].Splat->Refresh();
                    }
                }
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Splatter::Upload(ConstRef<Region> Region, Ref<Splatmap> Splat)
    {
        constexpr UInt32 kUnitPitch = Region::kUnitsPerX * Splatmap::kSlots;
        constexpr UInt32 kLinePitch = kMapSize * Splatmap::kSlots;

        Blob Payload = Blob::Allocate<UInt8>(kMapSize * kLinePitch);

        const Ptr<UInt8>      Texels  = Payload.GetData<UInt8>();
        const ConstPtr<UInt8> Weights = Splat.GetWeights().GetData();

        for (UInt16 Row = 0; Row < kMapSize; ++Row)
        {
            const SInt32     EdgeY = Clamp(Row - kMapBorder, 0, Region::kUnitsPerY - 1);
            const Ptr<UInt8> Line  = Texels + Row * kLinePitch;
            const Ptr<UInt8> First = Line + kMapBorder * Splatmap::kSlots;
            const Ptr<UInt8> Last  = Line + (kMapSize - kMapBorder - 1) * Splatmap::kSlots;

            Blit(First, kUnitPitch, Weights + EdgeY * kUnitPitch);

            for (UInt16 Column = 0; Column < kMapBorder; ++Column)
            {
                Blit(First - (Column + 1) * Splatmap::kSlots, Splatmap::kSlots, First);
                Blit(Last  + (Column + 1) * Splatmap::kSlots, Splatmap::kSlots, Last);
            }
        }

        // Holds one of the eight regions around this one, looked up once instead of once per gutter texel.
        struct Border final
        {
            ConstPtr<Splatmap>             Splat;
            Array<UInt8, Splatmap::kSlots> Remap;
        };

        Array<Border, 9> Borders { };

        for (SInt32 StepY = -1; StepY <= 1; ++StepY)
        {
            for (SInt32 StepX = -1; StepX <= 1; ++StepX)
            {
                const ConstPtr<UInt32> Slot = mIndices.Find(GetKey(Region.GetX() + StepX, Region.GetY() + StepY));

                if (!Slot)
                {
                    continue;
                }

                Ref<Border> Edge = Borders[ConvertTo1D<SInt32>(StepX + 1, StepY + 1, 3)];
                Edge.Splat = mRegions[* Slot].Splat;

                // A slot means a different slice in every region, so the neighbour's weights only mean
                // something here once they have been named and looked up in this region's own palette.
                for (UInt8 Source = 0; Source < Splatmap::kSlots; ++Source)
                {
                    Edge.Remap[Source] = Splatmap::kSlots;

                    for (UInt8 Target = 0; Target < Splatmap::kSlots; ++Target)
                    {
                        if (Splat.GetSplat(Target) == Edge.Splat->GetSplat(Source))
                        {
                            Edge.Remap[Source] = Target;
                            break;
                        }
                    }
                }
            }
        }

        for (SInt32 Row = 0; Row < kMapSize; ++Row)
        {
            const SInt32 UnitY = Row - kMapBorder;
            const SInt32 EdgeY = Clamp(UnitY, 0, Region::kUnitsPerY - 1);
            const SInt32 StepY = (UnitY < 0 ? -1 : (UnitY < Region::kUnitsPerY ? 0 : 1));

            for (SInt32 Column = 0; Column < kMapSize; ++Column)
            {
                const SInt32 UnitX = Column - kMapBorder;
                const SInt32 StepX = (UnitX < 0 ? -1 : (UnitX < Region::kUnitsPerX ? 0 : 1));

                // The interior is this region's own ground, already copied over, and borrows nothing.
                if (StepX == 0 && StepY == 0)
                {
                    continue;
                }

                ConstRef<Border> Edge = Borders[ConvertTo1D<SInt32>(StepX + 1, StepY + 1, 3)];

                // A neighbour the world has not streamed in has nothing to say about the edge, so the
                // stretched fallback stands.
                if (!Edge.Splat)
                {
                    continue;
                }

                const SInt32 EdgeX  = Clamp(UnitX, 0, Region::kUnitsPerX - 1);
                const UInt8  OtherX = static_cast<UInt8>(EdgeX - StepX * (Region::kUnitsPerX - 1));
                const UInt8  OtherY = static_cast<UInt8>(EdgeY - StepY * (Region::kUnitsPerY - 1));

                Array<UInt16, Splatmap::kSlots> Blend { };

                UInt32 Carried = 0;

                for (UInt8 Source = 0; Source < Splatmap::kSlots; ++Source)
                {
                    if (const UInt8 Target = Edge.Remap[Source]; Target < Splatmap::kSlots)
                    {
                        const UInt8 Weight = Edge.Splat->GetWeight(OtherX, OtherY, Source);

                        Blend[Target] += Weight;
                        Carried       += Weight;
                    }
                }

                const Ptr<UInt8> Texel   = Texels + Row * kLinePitch + Column * Splatmap::kSlots;
                const UInt32     Missing = (Carried < 255 ? 255 - Carried : 0);

                for (UInt8 Slot = 0; Slot < Splatmap::kSlots; ++Slot)
                {
                    const UInt32 Total = Blend[Slot] + (Texel[Slot] * Missing) / 255;

                    Texel[Slot] = static_cast<UInt8>(Total < 255 ? Total : 255);
                }
            }
        }

        const UInt16 Page = Splat.GetPage();

        mService->UpdateTexture(
            mTextures[Page / kPage],
            0,
            Page % kPage,
            0,
            0,
            kMapSize,
            kMapSize,
            kLinePitch,
            Move(Payload));

        Splat.Validate();
    }
}