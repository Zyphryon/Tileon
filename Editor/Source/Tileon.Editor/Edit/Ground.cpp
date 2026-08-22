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

#include "Ground.hpp"
#include "Tileon.Render/Component.hpp"
#include "Tileon.World/Component/Lifecycle.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Real32 Grain(IntVector2 Unit)
    {
        UInt32 Seed = static_cast<UInt32>(Unit.GetX()) * 73856093u ^ static_cast<UInt32>(Unit.GetY()) * 19349663u;

        Seed ^= Seed >> 13;
        Seed *= 0x5BD1E995u;
        Seed ^= Seed >> 15;

        return static_cast<Real32>(Seed & 0xFF) * (1.0f / 255.0f);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Real32 Threshold(IntVector2 Unit)
    {
        static constexpr UInt8 kOrdered[16] = {
            0,  8,  2, 10,
           12,  4, 14,  6,
            3, 11,  1,  9,
           15,  7, 13,  5 };

        const UInt32 Column = static_cast<UInt32>(Unit.GetX()) & 3u;
        const UInt32 Row    = static_cast<UInt32>(Unit.GetY()) & 3u;

        return static_cast<Real32>(kOrdered[Row * 4 + Column]) * (1.0f / 16.0f);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ground::Ground(Ref<Context> Context)
        : mContext { Context },
          mShape   { Shape::Circle },
          mSize    { 3 },
          mFlow    { 255 },
          mFalloff { Falloff::Linear }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt8 Ground::Cover(IntVector2 Unit, IntVector2 Offset) const
    {
        const Real32 X     = static_cast<Real32>(Abs(Offset.GetX()));
        const Real32 Y     = static_cast<Real32>(Abs(Offset.GetY()));
        const Real32 Reach = static_cast<Real32>(mSize);

        Real32 Distance = 0.0f;

        switch (mShape)
        {
        case Shape::Square:
            Distance = Max(X, Y);
            break;
        case Shape::Diamond:
            Distance = X + Y;
            break;
        default:
            Distance = Sqrt(X * X + Y * Y);
            break;
        }

        if (Distance >= Reach)
        {
            return 0;
        }

        Real32 Share;

        if (mShape == Shape::Ring)
        {
            // The rim keeps a band of its own and peaks along the middle of it, so the disc is left hollow.
            const Real32 Band   = Max(Reach * 0.5f, 1.0f);
            const Real32 Middle = Reach - Band * 0.5f;

            Share = 1.0f - Abs(Distance - Middle) / Max(Band * 0.5f, 1.0f);
        }
        else
        {
            Share = (Reach - Distance) / Max(Reach * 0.5f, 1.0f);
        }

        Share = Clamp(Share, 0.0f, 1.0f);

        switch (mFalloff)
        {
        case Falloff::Hard:
            Share = (Share > 0.0f ? 1.0f : 0.0f);
            break;
        case Falloff::Linear:
            break;
        case Falloff::Smooth:
            Share = Share * Share * (3.0f - 2.0f * Share);
            break;
        case Falloff::Dither:
            Share = (Share > Threshold(Unit) ? 1.0f : 0.0f);
            break;
        }

        if (mShape == Shape::Noise)
        {
            Share *= Grain(Unit);
        }
        return static_cast<UInt8>(Share * 255.0f);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Ground::Execute(Brush Brush, Command Command, Placement Placement, UInt32 Object)
    {
        IntRect    Area;
        IntVector2 Centre;

        switch (Brush)
        {
        case Brush::Hand:
        case Brush::Select:
            return;
        case Brush::Sample:
            Pick(Placement);
            return;
        case Brush::Smudge:
        case Brush::Pencil:
        {
            // The brush covers a shape around the unit under the cursor, so the stroke is worked out over
            // the square that shape fits inside and the shape itself decides what within it is touched.
            const SInt32 UnitX = Floor(Placement.GetAbsoluteX());
            const SInt32 UnitY = Floor(Placement.GetAbsoluteY());
            const SInt32 Reach = mSize - 1;

            Area   = IntRect(UnitX - Reach, UnitY - Reach, UnitX + Reach + 1, UnitY + Reach + 1);
            Centre = IntVector2(UnitX, UnitY);
            break;
        }
        case Brush::Bucket:
        {
            const SInt32 UnitX = Placement.GetRegionX() * Region::kUnitsPerX;
            const SInt32 UnitY = Placement.GetRegionY() * Region::kUnitsPerY;

            Area   = IntRect(UnitX, UnitY, UnitX + Region::kUnitsPerX, UnitY + Region::kUnitsPerY);
            Centre = IntVector2(UnitX, UnitY);
            break;
        }
        }

        Ref<Supervisor> Supervisor = mContext.GetSupervisor();

        // The same region walk the units take, since a stroke may straddle a boundary either way.
        const SInt16 RegionMinX = Coordinate::GetRegionX(Area.GetMinimumX());
        const SInt16 RegionMinY = Coordinate::GetRegionY(Area.GetMinimumY());
        const SInt16 RegionMaxX = Coordinate::GetRegionX(Area.GetMaximumX() - 1);
        const SInt16 RegionMaxY = Coordinate::GetRegionY(Area.GetMaximumY() - 1);

        for (SInt16 RegionY = RegionMinY; RegionY <= RegionMaxY; ++RegionY)
        {
            for (SInt16 RegionX = RegionMinX; RegionX <= RegionMaxX; ++RegionX)
            {
                const Scene::Entity Actor = Supervisor.GetOrLoadRegion(RegionX, RegionY, true);

                if (!Actor.IsValid())
                {
                    continue;
                }

                if (!Actor.Has<Region>())
                {
                    Ref<Deferred> Waiting = mDeferred.Append();
                    Waiting.RegionX = RegionX;
                    Waiting.RegionY = RegionY;
                    Waiting.Brush   = Brush;
                    Waiting.Command = Command;
                    Waiting.Area    = Area;
                    Waiting.Centre  = Centre;
                    Waiting.Slice   = static_cast<UInt16>(Object);
                    continue;
                }

                Ptr<Splatmap> Surface = Actor.TryGet<Splatmap>();

                if (!Surface)
                {
                    // Erasing has nothing to take away from a region nothing was ever laid on.
                    if (Command == Command::Remove)
                    {
                        continue;
                    }

                    Surface = static_cast<Ptr<Splatmap>>(Actor.Ensure<Splatmap>());
                }

                Apply(Actor, Surface, Brush, Command, Area, Centre, static_cast<UInt16>(Object));
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Ground::Apply(
        Scene::Entity Actor,
        Ptr<Splatmap> Surface,
        Brush         Brush,
        Command       Command,
        IntRect       Area,
        IntVector2    Centre,
        UInt16        Slice)
    {
        const Bool Mixing = (Brush == Brush::Smudge);

        if (!Mixing && Command == Command::Remove && !Surface->IsVisible(0))
        {
            return;
        }

        const UInt16 Paint = (Command == Command::Add ? Slice : Surface->GetSplat(0));

        ConstRef<Tileon::Region> Region = Actor.Get<const Tileon::Region>();

        const SInt32 OriginX = Region.GetX() * Tileon::Region::kUnitsPerX;
        const SInt32 OriginY = Region.GetY() * Tileon::Region::kUnitsPerY;

        const IntRect Clipped(
            Max(Area.GetMinimumX(), OriginX),
            Max(Area.GetMinimumY(), OriginY),
            Min(Area.GetMaximumX(), OriginX + Tileon::Region::kUnitsPerX),
            Min(Area.GetMaximumY(), OriginY + Tileon::Region::kUnitsPerY));

        // The bucket fills whatever it lands on evenly; every other brush wears a shape.
        const Bool Shaped = (Brush != Brush::Bucket);

        UInt8 Slot    = Splatmap::kSlots;
        Bool  Touched = false;

        for (SInt32 Y = Clipped.GetMinimumY(); Y < Clipped.GetMaximumY(); ++Y)
        {
            for (SInt32 X = Clipped.GetMinimumX(); X < Clipped.GetMaximumX(); ++X)
            {
                const UInt8 Covered = Shaped
                    ? Cover(IntVector2(X, Y), IntVector2(X - Centre.GetX(), Y - Centre.GetY()))
                    : 255;

                if (Covered == 0)
                {
                    continue;
                }

                const UInt32 Share    = Shaped ? Covered * mFlow / 255 : 255;
                const UInt8  Strength = static_cast<UInt8>(Share > 0 ? Share : 1);
                const UInt8  LocalX   = static_cast<UInt8>(X - OriginX);
                const UInt8  LocalY   = static_cast<UInt8>(Y - OriginY);

                if (Mixing)
                {
                    if (!Touched)
                    {
                        mContext.GetHistory().CaptureRegion(Actor);
                        Touched = true;
                    }

                    Soften(Surface, LocalX, LocalY, Strength);
                    continue;
                }

                if (Slot == Splatmap::kSlots)
                {
                    mContext.GetHistory().CaptureRegion(Actor);

                    if (Slot = Surface->Claim(Paint); Slot == Splatmap::kSlots)
                    {
                        LOG_W("Ground: region {0},{1} already blends four terrains", Region.GetX(), Region.GetY());
                        return;
                    }
                    Touched = true;
                }

                Surface->Blend(LocalX, LocalY, Slot, Strength);
            }
        }

        if (!Touched)
        {
            return;
        }

        // Mark the region as dirty so the paint gets saved and reloaded with it.
        Actor.Add<Persist>();

        // The weights were mutated in place, so signal the change for the render-side ground cache.
        Actor.Notify<Splatmap>();

        // Every map carries a ring of its neighbours' weights, so a stroke that reached a border leaves those
        // neighbours to rebuild as well.
        const Bool Bordered = Clipped.GetMinimumX() == OriginX
                           || Clipped.GetMinimumY() == OriginY
                           || Clipped.GetMaximumX() == OriginX + Tileon::Region::kUnitsPerX
                           || Clipped.GetMaximumY() == OriginY + Tileon::Region::kUnitsPerY;
        if (!Bordered)
        {
            Surface->Settle();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Ground::Soften(Ptr<Splatmap> Surface, UInt8 X, UInt8 Y, UInt8 Strength)
    {
        const SInt32 MinX = Max<SInt32>(X - 1, 0);
        const SInt32 MinY = Max<SInt32>(Y - 1, 0);
        const SInt32 MaxX = Min<SInt32>(X + 1, Tileon::Region::kUnitsPerX - 1);
        const SInt32 MaxY = Min<SInt32>(Y + 1, Tileon::Region::kUnitsPerY - 1);

        Array<UInt32, Splatmap::kSlots> Total { };

        UInt32 Count = 0;

        // The ring stops at the region's edge, so a stroke along a border leans on the side it can see.
        for (SInt32 NeighbourY = MinY; NeighbourY <= MaxY; ++NeighbourY)
        {
            for (SInt32 NeighbourX = MinX; NeighbourX <= MaxX; ++NeighbourX)
            {
                for (UInt8 Slot = 0; Slot < Splatmap::kSlots; ++Slot)
                {
                    Total[Slot] += Surface->GetWeight(
                        static_cast<UInt8>(NeighbourX), static_cast<UInt8>(NeighbourY), Slot);
                }
                ++Count;
            }
        }

        Array<UInt8, Splatmap::kSlots> Mixed;

        for (UInt8 Slot = 0; Slot < Splatmap::kSlots; ++Slot)
        {
            const UInt32 Average = Total[Slot] / Count;
            const UInt32 Current = Surface->GetWeight(X, Y, Slot);

            Mixed[Slot] = static_cast<UInt8>((Current * (255 - Strength) + Average * Strength) / 255);
        }

        Surface->SetWeights(X, Y, Mixed);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Ground::Pick(Placement Placement)
    {
        const SInt32 UnitX = Floor(Placement.GetAbsoluteX());
        const SInt32 UnitY = Floor(Placement.GetAbsoluteY());

        const Scene::Entity Actor = mContext.GetSupervisor().GetOrLoadRegion(
            Coordinate::GetRegionX(UnitX), Coordinate::GetRegionY(UnitY), false);

        const ConstPtr<Splatmap> Surface = Actor.IsValid() ? Actor.TryGet<const Splatmap>() : nullptr;

        if (!Surface)
        {
            return;
        }

        const UInt8 LocalX = static_cast<UInt8>(UnitX - Coordinate::GetRegionX(UnitX) * Tileon::Region::kUnitsPerX);
        const UInt8 LocalY = static_cast<UInt8>(UnitY - Coordinate::GetRegionY(UnitY) * Tileon::Region::kUnitsPerY);

        UInt8 Chosen    = Splatmap::kSlots;
        UInt8 Strongest = 0;

        for (UInt8 Slot = 0; Slot < Splatmap::kSlots; ++Slot)
        {
            if (const UInt8 Weight = Surface->GetWeight(LocalX, LocalY, Slot); Weight > Strongest)
            {
                Strongest = Weight;
                Chosen    = Slot;
            }
        }

        if (Chosen < Splatmap::kSlots)
        {
            mContext.SetInteger(Session::kSelectionTerrain, Surface->GetSplat(Chosen));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Ground::Flush()
    {
        if (mDeferred.IsEmpty())
        {
            return;
        }

        Ref<Supervisor> Supervisor = mContext.GetSupervisor();

        for (UInt32 Index = 0; Index < mDeferred.GetSize(); )
        {
            ConstRef<Deferred> Waiting = mDeferred[Index];

            const Scene::Entity Actor = Supervisor.GetRegion(Waiting.RegionX, Waiting.RegionY);

            // Still on its way, so the stroke keeps waiting; a region that never arrives at all is dropped
            // rather than kept forever.
            if (Actor.IsValid() && !Actor.Has<Region>())
            {
                ++Index;
                continue;
            }

            if (Actor.IsValid())
            {
                Ptr<Splatmap> Surface = Actor.TryGet<Splatmap>();

                if (!Surface && Waiting.Command != Command::Remove)
                {
                    Surface = static_cast<Ptr<Splatmap>>(Actor.Ensure<Splatmap>());
                }

                if (Surface)
                {
                    Apply(Actor, Surface, Waiting.Brush, Waiting.Command, Waiting.Area, Waiting.Centre, Waiting.Slice);
                }
            }

            mDeferred.Remove(Index);
        }
    }
}