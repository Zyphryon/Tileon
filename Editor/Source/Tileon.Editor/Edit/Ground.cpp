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

    Ground::Ground(Ref<Context> Context)
        : mContext { Context },
          mShape   { Shape::Circle },
          mSize    { 3 },
          mFlow    { 255 },
          mSoft    { true }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt8 Ground::Cover(SInt32 OffsetX, SInt32 OffsetY) const
    {
        const Real32 X = static_cast<Real32>(Abs(OffsetX));
        const Real32 Y = static_cast<Real32>(Abs(OffsetY));

        Real32 Distance = 0.0f;

        switch (mShape)
        {
        case Shape::Square:
            Distance = Max(X, Y);
            break;
        case Shape::Circle:
            Distance = Sqrt(X * X + Y * Y);
            break;
        case Shape::Diamond:
            Distance = X + Y;
            break;
        }

        if (Distance >= mSize)
        {
            return 0;
        }

        if (!mSoft)
        {
            return 255;
        }

        // A soft brush holds full strength over its inner half and gives way over the outer one, which
        // leaves a band wide enough to read as a blend without washing the middle of the stroke out.
        const Real32 Falloff = Max(mSize * 0.5f, 1.0f);
        const Real32 Share   = Clamp((mSize - Distance) / Falloff, 0.0f, 1.0f);
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
        if (Command == Command::Remove && !Surface->IsVisible(0))
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

        UInt8 Slot = Splatmap::kSlots;

        for (SInt32 Y = Clipped.GetMinimumY(); Y < Clipped.GetMaximumY(); ++Y)
        {
            for (SInt32 X = Clipped.GetMinimumX(); X < Clipped.GetMaximumX(); ++X)
            {
                const UInt8 Covered = Shaped ? Cover(X - Centre.GetX(), Y - Centre.GetY()) : 255;

                if (Covered == 0)
                {
                    continue;
                }

                const UInt32 Share    = Shaped ? Covered * mFlow / 255 : 255;
                const UInt8  Strength = static_cast<UInt8>(Share > 0 ? Share : 1);

                if (Slot == Splatmap::kSlots)
                {
                    mContext.GetHistory().CaptureRegion(Actor);

                    if (Slot = Surface->Claim(Paint); Slot == Splatmap::kSlots)
                    {
                        LOG_W("Ground: region {0},{1} already blends four terrains", Region.GetX(), Region.GetY());
                        return;
                    }
                }

                Surface->Blend(static_cast<UInt8>(X - OriginX), static_cast<UInt8>(Y - OriginY), Slot, Strength);
            }
        }

        if (Slot == Splatmap::kSlots)
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
                    Apply(Actor, Surface, Brush::Pencil, Waiting.Command, Waiting.Area, Waiting.Centre, Waiting.Slice);
                }
            }

            mDeferred.Remove(Index);
        }
    }
}