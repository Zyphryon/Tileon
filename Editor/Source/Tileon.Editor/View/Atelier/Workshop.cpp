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
#include "Tileon.World/Component.hpp"
#include "Tileon.World/Component/State/Lifecycle.hpp"
#include <Zyphryon.Base/Marshal/Binary/Reader.hpp>
#include <Zyphryon.Base/Marshal/Binary/Writer.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Workshop::Workshop(Ref<Context> Context)
        : mContext          { Context },
          mMode             { Mode::Tile },
          mLevel            { Level::Base },
          mBrush            { Brush::Pencil },
          mSelectionPrimary { 0 },
          mClipboardCount   { 0 }
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

    void Workshop::UpdatePreview(Placement Placement, UInt32 Object)
    {
        if (mMode != Mode::Entity || mBrush != Brush::Pencil || Object == 0)
        {
            ClearPreview();
            return;
        }

        const Scene::Entity Archetype = mContext.GetRepository().GetArchetype(Scene::kMinRangeArchetypes + Object);

        // Unlike placing, merely hovering must never bring a region into existence.
        const Scene::Entity Actor = mContext.GetSupervisor().GetRegion(Placement.GetRegionX(), Placement.GetRegionY());

        if (!Archetype.IsValid() || !Actor.IsValid())
        {
            ClearPreview();
            return;
        }

        EnsurePreview(Actor, Archetype, Placement);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::ClearPreview()
    {
        if (mPreview.IsAlive())
        {
            mPreview.Add<Dispose>();
        }
        mPreview = Scene::Entity();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Entity Workshop::EnsurePreview(Scene::Entity Actor, Scene::Entity Archetype, Placement Placement)
    {
        if (!mPreview.IsAlive() || mPreview.GetArchetype() != Archetype)
        {
            ClearPreview();

            mPreview = mContext.GetScene().CreateEntity();
            mPreview.SetArchetype(Archetype);

            Scene::Entity::AddRecursively<Unpickable>(mPreview);

            // Fade the archetype's own tint, so the preview reads as not-yet-placed.
            const ConstPtr<IntColor8> Tint = Archetype.TryGet<const IntColor8>();
            const IntColor8           Base = Tint ? (* Tint) : IntColor8::White();
            mPreview.Set(IntColor8(Base.GetRed(), Base.GetGreen(), Base.GetBlue(), 140));
        }

        if (mPreview.GetParent() != Actor)
        {
            mPreview.Attach(Actor, Scene::Hierarchy::Open);
        }

        // Only the translation tracks the cursor; the scale and rotation the user dialed in stay untouched.
        if (const Ptr<Pose> Pose = mPreview.TryGet<Tileon::Pose>())
        {
            Pose->SetTranslation(Vector2(Placement.GetOffsetX(), Placement.GetOffsetY()));
        }
        else
        {
            mPreview.Set(Tileon::Pose(Vector2(Placement.GetOffsetX(), Placement.GetOffsetY())));
        }

        mPreview.Add<Stale>();
        return mPreview;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::ExecuteOnTiles(Command Command, Placement Placement, UInt32 Object)
    {
        // Gets the tileset entry for the specified object ID and retrieves its column and row span.
        const IntVector2 Span  = Object ? mContext.GetTileset().GetMotif(Object).GetSpan() : IntVector2::One();

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
        switch (mBrush)
        {
        case Brush::Hand:
        case Brush::Select:
        {
            if (Command == Command::Add)
            {
                SelectEntity(Placement);
            }
            break;
        }
        case Brush::Bucket:
            break;
        case Brush::Pencil:
            if (Command == Command::Add)
            {
                AddEntity(Placement, Object);
            }
            else
            {
                RemoveEntity(Placement);
            }
            break;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::AddEntity(Placement Placement, UInt32 Object)
    {
        const Scene::Entity Archetype = mContext.GetRepository().GetArchetype(Scene::kMinRangeArchetypes + Object);

        if (!Archetype.IsValid())
        {
            return;
        }

        const Scene::Entity Actor = mContext.GetSupervisor().GetOrLoadRegion(
            Placement.GetRegionX(),
            Placement.GetRegionY(), true);

        if (!Actor.IsValid())
        {
            return;
        }

        // Place whatever the preview was showing, so the result is exactly what sat under the cursor.
        const Scene::Entity Instance = EnsurePreview(Actor, Archetype, Placement);

        // Promote the preview into a placed entity by granting back everything it was deliberately denied.
        Instance.Remove<IntColor8>();
        Instance.Add<Stale>();
        Instance.Add<Persist>();

        Scene::Entity::RemoveRecursively<Unpickable>(Instance);

        // Mark the region as dirty so it gets saved and reloaded with the placed entity.
        Actor.Add<Persist>();

        // Select what was just placed so the inspector targets it; the brush stays armed for the next stamp.
        mContext.SetInteger("Selection.Entity", Instance.GetID());

        // Snapshot the placed pose so the replacement preview continues from the same scale and rotation.
        mPreview = Scene::Entity();
        EnsurePreview(Actor, Archetype, Placement);

        Pose Transformation = Instance.Get<Pose>();
        mPreview.Set(Move(Transformation));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::RemoveEntity(Placement Placement)
    {
        if (Scene::Entity Instance = PickEntity(Placement); Instance.IsValid())
        {
            Instance = Scene::Entity::ResolveRecursively(Instance, Scene::Hierarchy::Fixed);
            Instance.Add<Dispose>();

            if (const Scene::Entity Actor = Instance.GetParent(); Actor.IsValid())
            {
                Actor.Add<Persist>();
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::SelectEntity(Placement Placement)
    {
        SelectSingle(Placement);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::SetPrimary(UInt64 Entity)
    {
        mSelectionPrimary = Entity;
        mContext.SetInteger("Selection.Entity", static_cast<SInt64>(Entity));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Placement Workshop::OriginOf(Scene::Entity Actor) const
    {
        Vector2 Local = Vector2::Zero();

        if (const ConstPtr<Pose> Component = Actor.TryGet<const Tileon::Pose>())
        {
            Local = Component->GetTranslation();
        }

        SInt16 RegionX = 0;
        SInt16 RegionY = 0;

        if (const Scene::Entity Region = Actor.GetParent(Scene::Hierarchy::Open); Region.IsValid())
        {
            if (const ConstPtr<Tileon::Region> Component = Region.TryGet<const Tileon::Region>())
            {
                RegionX = Component->GetX();
                RegionY = Component->GetY();
            }
        }
        return Placement(RegionX, RegionY, Local.GetX(), Local.GetY());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::ReconcileSelection()
    {
        const UInt64 Current = static_cast<UInt64>(mContext.GetInteger("Selection.Entity", 0));

        if (Current != mSelectionPrimary)
        {
            mSelection.Clear();

            if (Current)
            {
                mSelection.Insert(Current);
            }
            mSelectionPrimary = Current;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::ClearSelection()
    {
        mSelection.Clear();
        SetPrimary(0);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::SelectSingle(Placement Placement)
    {
        mSelection.Clear();

        if (Scene::Entity Actor = PickEntity(Placement); Actor.IsValid())
        {
            Actor = Scene::Entity::ResolveRecursively(Actor, Scene::Hierarchy::Fixed);
            mSelection.Insert(Actor.GetID());
            SetPrimary(Actor.GetID());
        }
        else
        {
            SetPrimary(0);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::SelectToggle(Placement Placement)
    {
        Scene::Entity Actor = PickEntity(Placement);

        if (!Actor.IsValid())
        {
            return;
        }
        Actor = Scene::Entity::ResolveRecursively(Actor, Scene::Hierarchy::Fixed);

        if (const UInt64 ID = Actor.GetID(); mSelection.Contains(ID))
        {
            mSelection.Erase(ID);

            if (mSelectionPrimary == ID)
            {
                UInt64 Next = 0;
                for (const UInt64 Remaining : mSelection)
                {
                    Next = Remaining;
                    break;
                }
                SetPrimary(Next);
            }
        }
        else
        {
            mSelection.Insert(ID);
            SetPrimary(ID);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::SelectWithin(IntRect Area, Bool Additive)
    {
        if (!Additive)
        {
            mSelection.Clear();
        }

        UInt64 Primary = mSelectionPrimary;

        mContext.GetSupervisor().QueryEach(Area, [&](Scene::Entity Actor)
        {
            if (!Actor.IsValid() || Actor.Has<Unpickable>())
            {
                return;
            }

            // Cells are coarse, so confirm the entity's own bound actually overlaps the marquee.
            if (const ConstPtr<Bound> Volume = Actor.TryGet<const Tileon::Bound>())
            {
                if (!Volume->GetRect().Test(Area))
                {
                    return;
                }
            }

            const Scene::Entity Root = Scene::Entity::ResolveRecursively(Actor, Scene::Hierarchy::Fixed);
            mSelection.Insert(Root.GetID());
            Primary = Root.GetID();
        });

        SetPrimary(mSelection.IsEmpty() ? 0 : Primary);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::DeleteSelection()
    {
        Ref<Scene::Service> Scene = mContext.GetScene();

        for (const UInt64 ID : mSelection)
        {
            Scene::Entity Actor = Scene.GetEntity(ID);

            if (!Actor.IsValid())
            {
                continue;
            }
            Actor = Scene::Entity::ResolveRecursively(Actor, Scene::Hierarchy::Fixed);

            if (const Scene::Entity Region = Actor.GetParent(Scene::Hierarchy::Open); Region.IsValid())
            {
                Region.Add<Persist>();
            }
            Actor.Add<Dispose>();
        }
        ClearSelection();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::CopySelection()
    {
        Ref<Scene::Service> Scene = mContext.GetScene();

        struct Item final
        {
            Scene::Entity Root;
            Placement     Origin;
        };

        Sequence<Item> Items;
        Real64         SumX = 0.0;
        Real64         SumY = 0.0;

        for (const UInt64 ID : mSelection)
        {
            Scene::Entity Root = Scene.GetEntity(ID);

            if (!Root.IsValid())
            {
                continue;
            }
            Root = Scene::Entity::ResolveRecursively(Root, Scene::Hierarchy::Fixed);

            const Placement Origin = OriginOf(Root);
            Items.Append(Item { Root, Origin });

            SumX += Origin.GetAbsoluteX();
            SumY += Origin.GetAbsoluteY();
        }

        if (Items.IsEmpty())
        {
            mClipboardCount = 0;
            return;
        }

        // Anchor the clipboard on the group's centroid, so a paste re-centres the whole group on the cursor.
        const Real64 AnchorX = SumX / Items.GetSize();
        const Real64 AnchorY = SumY / Items.GetSize();

        Writer Output;
        Output.Write<UInt32>(static_cast<UInt32>(Items.GetSize()));

        for (ConstRef<Item> Entry : Items)
        {
            Output.Write<Real64>(Entry.Origin.GetAbsoluteX() - AnchorX);
            Output.Write<Real64>(Entry.Origin.GetAbsoluteY() - AnchorY);
            Scene.SaveHierarchy(Output, Entry.Root);
        }

        mClipboard      = Output.Detach();
        mClipboardCount = static_cast<UInt32>(Items.GetSize());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::CutSelection()
    {
        CopySelection();
        DeleteSelection();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::Paste(Placement Placement)
    {
        if (mClipboardCount == 0 || mClipboard == nullptr)
        {
            return;
        }

        Ref<Scene::Service> Scene      = mContext.GetScene();
        Ref<Supervisor>     Supervisor = mContext.GetSupervisor();

        Reader Input(mClipboard.GetData(), mClipboard.GetSize());
        const UInt32 Count = Input.Read<UInt32>();

        mSelection.Clear();
        UInt64 Primary = 0;

        for (UInt32 Element = 0; Element < Count; ++Element)
        {
            const Real64 OffsetX = Input.Read<Real64>();
            const Real64 OffsetY = Input.Read<Real64>();

            Scene::Entity Actor = Scene.LoadHierarchy(Input);

            if (!Actor.IsValid())
            {
                continue;
            }

            const Tileon::Placement Target = Tileon::Placement::FromAbsolute(
                Placement.GetAbsoluteX() + OffsetX, Placement.GetAbsoluteY() + OffsetY);

            Scene::Entity Region = Supervisor.GetOrLoadRegion(Target.GetRegionX(), Target.GetRegionY(), true);

            if (!Region.IsValid())
            {
                Actor.Add<Dispose>();
                continue;
            }

            // The subtree kept its original scale and rotation; only the translation moves to the paste target.
            Pose Transformation = Actor.TryGet<const Tileon::Pose>() ? Actor.Get<const Tileon::Pose>() : Pose();
            Transformation.SetTranslation(Vector2(Target.GetOffsetX(), Target.GetOffsetY()));
            Actor.Set(Move(Transformation));

            Actor.Attach(Region, Scene::Hierarchy::Open);
            Actor.Add<Stale>();
            Actor.Add<Persist>();
            Region.Add<Persist>();

            mSelection.Insert(Actor.GetID());
            Primary = Actor.GetID();
        }
        SetPrimary(Primary);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Workshop::ApplyTiles(Command Command, IntRect Area, Tile::Layer Layer, UInt16 Handle, IntVector2 Span)
    {
        Ref<Supervisor> Supervisor = mContext.GetSupervisor();

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
            // Align the atlas to the world grid, not to where the stamp began. A cell's sub-tile is then a pure
            // function of its global position, so a terrain tiles seamlessly and overlapping strokes stay
            // consistent instead of each stamp restarting the pattern and overwriting the last one's weight.
            const SInt32 SpanX = Operation.Span.GetX();
            const SInt32 SpanY = Operation.Span.GetY();

            const IntVector2 Offset(
                ((GlobalClipMinX % SpanX) + SpanX) % SpanX,
                ((GlobalClipMinY % SpanY) + SpanY) % SpanY);

            Region->Fill(LocalArea, Operation.Layer, Operation.Terrain, Operation.Span, Offset);
        }
        else
        {
            Region->Erase(LocalArea, Operation.Layer);
        }

        // Mark the region as dirty so it gets saved and reloaded with the updated tile data.
        Operation.Actor.Add<Persist>();

        // The tiles were mutated in place, so signal the change for the render-side block cache.
        Operation.Actor.Notify<Tileon::Region>();
    }
}