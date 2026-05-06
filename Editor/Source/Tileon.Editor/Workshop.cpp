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
        // Validates that the specified object ID corresponds to a valid tileset entry.
        Ref<Tileset> Tileset = mController.GetRenderer().GetTileset();
        if (!Tileset.HasEntry(Object))
        {
            LOG_WARNING("Invalid tileset entry ID: {}", Object);
            return;
        }

        // Gets the tileset entry for the specified object ID and retrieves its column and row span.
        ConstRef<Tileset::Entry> Entry = Tileset.GetEntry(Object);
        const IntVector2 Span(Entry.Columns, Entry.Rows);

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
            const IntRect Area(TileX, TileY, TileX + Entry.Columns, TileY + Entry.Rows);

            ApplyTiles(Command, Area, static_cast<Tile::Layer>(Enum::Cast(mLevel)), Object, Span);
            break;
        }
        case Brush::Bucket:
        {
            const SInt32  TileX = Placement.GetRegionX() * Region::kTilesPerX;
            const SInt32  TileY = Placement.GetRegionY() * Region::kTilesPerX;
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
        const SInt16 RegionMaxX = Coordinate::GetRegionX(Area.GetMaximumX());
        const SInt16 RegionMaxY = Coordinate::GetRegionY(Area.GetMaximumY());

        for (SInt16 RegionY = RegionMinY; RegionY <= RegionMaxY; ++RegionY)
        {
            for (SInt16 RegionX = RegionMinX; RegionX <= RegionMaxX; ++RegionX)
            {
                Scene::Entity Actor = Supervisor.GetOrLoadRegion(RegionX, RegionY, true);

                if (Actor.IsValid())
                {
                    // Compute this region's global tile origin.
                    const SInt32 OriginX = RegionX * Region::kTilesPerX;
                    const SInt32 OriginY = RegionY * Region::kTilesPerY;

                    // Clip the global area to this region's tile range and convert to region-local coordinates.
                    const SInt32 GlobalClipMinX = Max(Area.GetMinimumX(), OriginX);
                    const SInt32 GlobalClipMinY = Max(Area.GetMinimumY(), OriginY);
                    const SInt32 GlobalClipMaxX = Min(Area.GetMaximumX(), OriginX + Region::kTilesPerX);
                    const SInt32 GlobalClipMaxY = Min(Area.GetMaximumY(), OriginY + Region::kTilesPerY);

                    const IntRect LocalArea(
                        GlobalClipMinX - OriginX,
                        GlobalClipMinY - OriginY,
                        GlobalClipMaxX - OriginX,
                        GlobalClipMaxY - OriginY);

                    if (Command == Command::Add)
                    {
                        const IntVector2 Offset(
                            (GlobalClipMinX - Area.GetMinimumX()) % Span.GetX(),
                            (GlobalClipMinY - Area.GetMinimumY()) % Span.GetY());

                        Actor.Get<Region>().Fill(LocalArea, Layer, Handle, Span, Offset);
                    }
                    else
                    {
                        Actor.Get<Region>().Erase(LocalArea, Layer);
                    }
                }
            }
        }
    }
}