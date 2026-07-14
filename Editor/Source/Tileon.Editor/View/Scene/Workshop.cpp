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

#include "Workshop.hpp"
#include "Tileon.World/Component/State/Lifecycle.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Workshop::Workshop(Ref<Controller> Controller)
        : mController { Controller },
          mMode       { Mode::Tile },
          mLevel      { Level::Base },
          mBrush      { Brush::Pencil }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::Tick()
    {
        mOperations.RemoveFastSomeIf([&](ConstRef<OpTile> Operation) -> Bool
        {
            if (Operation.Actor.IsValid())
            {
                if (const Ptr<Region> Component = Operation.Actor.TryGet<Region>())
                {
                    ApplyTiles(Component, Operation);
                    return true;
                }
            }
            else
            {
                return true;
            }
            return false;
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::Execute(Command Command, Placement Placement, UInt32 Object)
    {
        switch (mMode)
        {
        case Mode::Tile:
            ExecuteOnTiles(Command, Placement, Object);
            break;
        case Mode::Entity:
            ExecuteOnEntities(Command, Placement, Object);
            break;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::ExecuteOnTiles(Command Command, Placement Placement, UInt32 Object)
    {
        Ref<Tileset> Tileset = mController.GetRenderer().GetTileset();

        // Gets the tileset entry for the specified object ID and retrieves its column and row span.
        IntVector2 Span = IntVector2::One();

        if (Object)
        {
            ConstRef<Motif> Motif = Tileset.GetMotif(Object);
            Span = Motif.GetSpan();
        }

        // Apply the command based on the current brush type and the placement of the cursor in the world.
        switch (mBrush)
        {
        case Brush::Hand:
        case Brush::Select:
            break;
        case Brush::Pencil:
        {
            const SInt32  TileX = Floor(Placement.GetAbsoluteX());
            const SInt32  TileY = Floor(Placement.GetAbsoluteY());
            const IntRect Area(TileX, TileY, TileX + Span.GetX(), TileY + Span.GetY());

            ApplyTiles(Command, Area, static_cast<Tile::Layer>(Enum::Cast(mLevel)), Object, Span);
            break;
        }
        case Brush::Bucket:
        {
            const SInt32  TileX = Placement.GetRegionX() * Region::kTilesPerX;
            const SInt32  TileY = Placement.GetRegionY() * Region::kTilesPerY;
            const IntRect Area(TileX, TileY, TileX + Region::kTilesPerX, TileY + Region::kTilesPerY);

            ApplyTiles(Command, Area, static_cast<Tile::Layer>(Enum::Cast(mLevel)), Object, Span);
        }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::ExecuteOnEntities(Command Command, Placement Placement, UInt32 Object)
    {
        // TODO: Missing Entities.
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::ApplyTiles(Command Command, IntRect Area, Tile::Layer Layer, UInt16 Handle, IntVector2 Span)
    {
        Ref<Supervisor> Supervisor = mController.GetWorld().GetSupervisor();

        // Compute the range of regions that the global tile rect overlaps.
        const SInt16 RegionMinX = Coordinate::GetRegionX(Area.GetMinimumX());
        const SInt16 RegionMinY = Coordinate::GetRegionY(Area.GetMinimumY());
        const SInt16 RegionMaxX = Coordinate::GetRegionX(Area.GetMaximumX() - 1);
        const SInt16 RegionMaxY = Coordinate::GetRegionY(Area.GetMaximumY() - 1);

        for (SInt16 RegionY = RegionMinY; RegionY <= RegionMaxY; ++RegionY)
        {
            for (SInt16 RegionX = RegionMinX; RegionX <= RegionMaxX; ++RegionX)
            {
                Scene::Entity Actor = Supervisor.GetOrLoadRegion(RegionX, RegionY, true);

                if (Actor.IsValid())
                {
                    const OpTile Operation(Actor, Command, Layer, Handle, Span, Area);

                    if (const Ptr<Region> Component = Actor.TryGet<Region>())
                    {
                        ApplyTiles(Component, Operation);
                    }
                    else
                    {
                        mOperations.Append(Operation);
                    }
                }
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::ApplyTiles(Ptr<Region> Region, ConstRef<OpTile> Operation)
    {
        // Compute this region's global tile origin.
        const SInt32 OriginX = Region->GetX() * Region::kTilesPerX;
        const SInt32 OriginY = Region->GetY() * Region::kTilesPerY;

        // Clip the global area to this region's tile range and convert to region-local coordinates.
        const SInt32 GlobalClipMinX = Max(Operation.Area.GetMinimumX(), OriginX);
        const SInt32 GlobalClipMinY = Max(Operation.Area.GetMinimumY(), OriginY);
        const SInt32 GlobalClipMaxX = Min(Operation.Area.GetMaximumX(), OriginX + Region::kTilesPerX);
        const SInt32 GlobalClipMaxY = Min(Operation.Area.GetMaximumY(), OriginY + Region::kTilesPerY);

        const IntRect LocalArea(
            GlobalClipMinX - OriginX,
            GlobalClipMinY - OriginY,
            GlobalClipMaxX - OriginX,
            GlobalClipMaxY - OriginY);

        if (Operation.Command == Command::Add)
        {
            const IntVector2 Offset(
                (GlobalClipMinX - Operation.Area.GetMinimumX()) % Operation.Span.GetX(),
                (GlobalClipMinY - Operation.Area.GetMinimumY()) % Operation.Span.GetY());

            Region->Fill(LocalArea, Operation.Layer, Operation.Terrain, Operation.Span, Offset);
        }
        else
        {
            Region->Erase(LocalArea, Operation.Layer);
        }

        // Mark the region as dirty so it gets saved and reloaded with the updated tile data.
        Operation.Actor.Add<Persist>();
    }
}