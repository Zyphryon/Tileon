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

#include "Tools.hpp"
#include "Tileon.Render/Component.hpp"
#include "Tileon.World/Component.hpp"
#include "Tileon.World/Component/Lifecycle.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Tools::Tools(Ref<Context> Context)
        : mContext          { Context },
          mBrush            { Brush::Pencil },
          mShape            { Shape::Circle },
          mSize             { 3 },
          mFlow             { 255 },
          mSoft             { true },
          mAligned          { false },
          mSelectionPrimary { 0 },
          mRevision         { 0 },
          mClipboardCount   { 0 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tools::Execute(Command Command, Placement Placement, UInt32 Object)
    {
        Ref<History> History = mContext.GetHistory();

        switch (GetMode())
        {
        case Mode::Entity:
            History.Open(Command == Command::Add ? "Place Entity"_Text : "Remove Entity"_Text);
            ExecuteOnEntities(Command, Placement, Object);
            History.Close();
            break;
        case Mode::Ground:
            History.Open("Paint Ground"_Text);
            ExecuteOnGround(Command, Placement, Object);
            History.Close();
            break;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tools::UpdatePreview(Placement Placement, UInt32 Object)
    {
        if (GetMode() != Mode::Entity || mBrush != Brush::Pencil || Object == 0)
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

    void Tools::ClearPreview()
    {
        if (mPreview.IsAlive())
        {
            mPreview.Add<Dispose>();
        }
        mPreview = Scene::Entity();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Entity Tools::EnsurePreview(Scene::Entity Actor, Scene::Entity Archetype, Placement Placement)
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
        const ConstPtr<Pose> Current   = mPreview.TryGet<const Tileon::Pose>();
        Real32               Elevation = Current ? Current->GetTranslation().GetY() : 0.0f;

        if (IsAlmostZero(Elevation))
        {
            if (const ConstPtr<Glowlight> Light = mPreview.TryGet<const Tileon::Glowlight>())
            {
                Elevation = Light->GetRadius() * kLightElevation;
            }
            else if (const ConstPtr<Spotlight> Light = mPreview.TryGet<const Tileon::Spotlight>())
            {
                Elevation = Light->GetRange() * kLightElevation;
            }
        }

        const Vector3 Position(Placement.GetOffsetX(), Elevation, Placement.GetOffsetY());

        if (const Ptr<Pose> Pose = mPreview.TryGet<Tileon::Pose>())
        {
            Pose->SetTranslation(Position);
        }
        else
        {
            mPreview.Set(Tileon::Pose(Position));
        }

        mPreview.Add<Stale>();
        return mPreview;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tools::ExecuteOnEntities(Command Command, Placement Placement, UInt32 Object)
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

    void Tools::AddEntity(Placement Placement, UInt32 Object)
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

        mContext.GetHistory().RecordEntity(Instance);

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

    void Tools::RemoveEntity(Placement Placement)
    {
        if (Scene::Entity Instance = PickEntity(Placement); Instance.IsValid())
        {
            Instance = Scene::Entity::ResolveRecursively(Instance, Scene::Hierarchy::Fixed);

            mContext.GetHistory().DiscardEntity(Instance);

            Instance.Add<Dispose>();

            if (const Scene::Entity Actor = Instance.GetParent(); Actor.IsValid())
            {
                Actor.Add<Persist>();
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tools::SelectEntity(Placement Placement)
    {
        SelectSingle(Placement);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tools::SetPrimary(UInt64 Entity)
    {
        mSelectionPrimary = Entity;
        mContext.SetInteger("Selection.Entity", static_cast<SInt64>(Entity));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Placement Tools::OriginOf(Scene::Entity Actor) const
    {
        Vector2 Local = Vector2::Zero();

        if (const ConstPtr<Pose> Component = Actor.TryGet<const Tileon::Pose>())
        {
            Local = Component->GetTranslation().GetXZ();
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

    void Tools::FlushGround()
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
                Ptr<Splatmap> Ground = Actor.TryGet<Splatmap>();

                if (!Ground && Waiting.Command != Command::Remove)
                {
                    Ground = static_cast<Ptr<Splatmap>>(Actor.Ensure<Splatmap>());
                }

                if (Ground)
                {
                    ApplyGround(Actor, Ground, Waiting.Command, Waiting.Area, Waiting.Centre, Waiting.Slice);
                }
            }

            mDeferred.Remove(Index);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tools::ReconcileSelection()
    {
        // Restoring an entity rebuilds it under a fresh id, so the ids the selection holds have to follow it.
        if (Ref<History> History = mContext.GetHistory(); History.GetRevision() != mRevision)
        {
            mRevision = History.GetRevision();

            Bag<UInt64> Live;

            for (const UInt64 ID : mSelection)
            {
                if (const UInt64 Remapped = History.Remap(ID))
                {
                    Live.Insert(Remapped);
                }
            }
            mSelection        = Move(Live);
            mSelectionPrimary = History.Remap(mSelectionPrimary);

            mContext.SetInteger("Selection.Entity", static_cast<SInt64>(mSelectionPrimary));
        }

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

    void Tools::ClearSelection()
    {
        mSelection.Clear();
        SetPrimary(0);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Entity Tools::ResolveSelection(Placement Placement)
    {
        // A part belongs to the instance it was built into, so a pick always lands on that root.
        if (const Scene::Entity Actor = PickEntity(Placement); Actor.IsValid())
        {
            return Scene::Entity::ResolveRecursively(Actor, Scene::Hierarchy::Fixed);
        }
        return Scene::Entity();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tools::SelectSingle(Placement Placement)
    {
        mSelection.Clear();

        if (const Scene::Entity Actor = ResolveSelection(Placement); Actor.IsValid())
        {
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

    void Tools::SelectToggle(Placement Placement)
    {
        const Scene::Entity Actor = ResolveSelection(Placement);

        if (!Actor.IsValid())
        {
            return;
        }

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

    void Tools::SelectWithin(ConstRef<Camera> Camera, ImVec2 Minimum, ImVec2 Maximum, Bool Additive)
    {
        if (!Additive)
        {
            mSelection.Clear();
        }

        const Placement Corner[4] =
        {
            Camera.Unproject(Minimum),
            Camera.Unproject(ImVec2(Maximum.x, Minimum.y)),
            Camera.Unproject(Maximum),
            Camera.Unproject(ImVec2(Minimum.x, Maximum.y)),
        };

        Real64 MinX = Corner[0].GetAbsoluteX(), MinY = Corner[0].GetAbsoluteY();
        Real64 MaxX = MinX, MaxY = MinY;

        for (UInt32 Index = 1; Index < 4; ++Index)
        {
            MinX = Min(MinX, Corner[Index].GetAbsoluteX());
            MinY = Min(MinY, Corner[Index].GetAbsoluteY());
            MaxX = Max(MaxX, Corner[Index].GetAbsoluteX());
            MaxY = Max(MaxY, Corner[Index].GetAbsoluteY());
        }

        const IntRect Area(
            static_cast<SInt32>(Floor(MinX)),     static_cast<SInt32>(Floor(MinY)),
            static_cast<SInt32>(Floor(MaxX)) + 1, static_cast<SInt32>(Floor(MaxY)) + 1);

        UInt64 Primary = mSelectionPrimary;

        const IntBox Swept(Area.GetMinimumX(), -static_cast<SInt32>(kPickElevation), Area.GetMinimumY(),
                           Area.GetMaximumX(),  static_cast<SInt32>(kPickElevation), Area.GetMaximumY());

        mContext.GetSupervisor().QueryEach(Swept, [&](Scene::Entity Actor)
        {
            if (!Actor.IsValid() || Actor.Has<Unpickable>() || !IsPickable(Actor))
            {
                return;
            }

            const ConstPtr<Enclosure> Volume = Actor.TryGet<const Tileon::Enclosure>();

            if (!Volume)
            {
                return;
            }

            const IntVector2 Center = Volume->GetVolume().GetCenter().GetXZ();
            const ImVec2     Screen = Camera.Project(Placement(0, 0, static_cast<Real32>(Center.GetX()), static_cast<Real32>(Center.GetY())));

            if (Screen.x < Minimum.x || Screen.x > Maximum.x || Screen.y < Minimum.y || Screen.y > Maximum.y)
            {
                return;
            }

            const Scene::Entity Root = Scene::Entity::ResolveRecursively(Actor, Scene::Hierarchy::Fixed);
            mSelection.Insert(Root.GetID());
            Primary = Root.GetID();
        });

        SetPrimary(mSelection.IsEmpty() ? 0 : Primary);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tools::DeleteSelection()
    {
        Ref<Scene::Service> Scene   = mContext.GetScene();
        Ref<History>        History = mContext.GetHistory();

        History.Open("Delete");

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
            History.DiscardEntity(Actor);

            Actor.Add<Dispose>();
        }
        History.Close();

        ClearSelection();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tools::CopySelection()
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
            Items.Append(Root, Origin);

            SumX += Origin.GetAbsoluteX();
            SumY += Origin.GetAbsoluteY();
        }

        if (Items.IsEmpty())
        {
            mClipboardCount = 0;
            return;
        }

        // Anchor the clipboard on the group's centroid, so a paste re-centers the whole group on the cursor.
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

    void Tools::CutSelection()
    {
        Ref<History> History = mContext.GetHistory();
        History.Open("Cut");

        CopySelection();
        DeleteSelection();

        History.Close();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tools::Paste(Placement Placement)
    {
        if (mClipboardCount == 0 || mClipboard == nullptr)
        {
            return;
        }

        Ref<Scene::Service> Scene      = mContext.GetScene();
        Ref<Supervisor>     Supervisor = mContext.GetSupervisor();
        Ref<History>        History    = mContext.GetHistory();

        History.Open("Paste");

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
            Transformation.SetTranslation(Vector3::FromXZ(Vector2(Target.GetOffsetX(), Target.GetOffsetY())));
            Actor.Set(Move(Transformation));

            Actor.Attach(Region, Scene::Hierarchy::Open);
            Actor.Add<Stale>();
            Actor.Add<Persist>();
            Region.Add<Persist>();

            History.RecordEntity(Actor);

            mSelection.Insert(Actor.GetID());
            Primary = Actor.GetID();
        }
        History.Close();

        SetPrimary(Primary);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt8 Tools::Cover(SInt32 OffsetX, SInt32 OffsetY) const
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

        const Real32 Reach = static_cast<Real32>(mSize);

        if (Distance >= Reach)
        {
            return 0;
        }

        if (!mSoft)
        {
            return 255;
        }

        // A soft brush holds full strength over its inner half and gives way over the outer one, which
        // leaves a band wide enough to read as a blend without washing the middle of the stroke out.
        const Real32 Falloff = Max(Reach * 0.5f, 1.0f);
        const Real32 Share   = Clamp((Reach - Distance) / Falloff, 0.0f, 1.0f);

        return static_cast<UInt8>(Share * 255.0f);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tools::ExecuteOnGround(Command Command, Placement Placement, UInt32 Object)
    {
        IntRect    Area;
        IntVector2 Centre;

        switch (mBrush)
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

                Ptr<Splatmap> Ground = Actor.TryGet<Splatmap>();

                if (!Ground)
                {
                    // Erasing has nothing to take away from a region nothing was ever laid on.
                    if (Command == Command::Remove)
                    {
                        continue;
                    }

                    Ground = static_cast<Ptr<Splatmap>>(Actor.Ensure<Splatmap>());
                }

                ApplyGround(Actor, Ground, Command, Area, Centre, static_cast<UInt16>(Object));
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tools::ApplyGround(Scene::Entity Actor, Ptr<Splatmap> Ground, Command Command, IntRect Area, IntVector2 Centre, UInt16 Slice)
    {
        if (Command == Command::Remove && !Ground->IsVisible(0))
        {
            return;
        }

        const UInt16 Paint = (Command == Command::Add ? Slice : Ground->GetSplat(0));

        ConstRef<Tileon::Region> Region = Actor.Get<const Tileon::Region>();

        const SInt32 OriginX = Region.GetX() * Tileon::Region::kUnitsPerX;
        const SInt32 OriginY = Region.GetY() * Tileon::Region::kUnitsPerY;

        const IntRect Clipped(
            Max(Area.GetMinimumX(), OriginX),
            Max(Area.GetMinimumY(), OriginY),
            Min(Area.GetMaximumX(), OriginX + Tileon::Region::kUnitsPerX),
            Min(Area.GetMaximumY(), OriginY + Tileon::Region::kUnitsPerY));

        // The bucket fills whatever it lands on evenly; every other brush wears a shape.
        const Bool Shaped = (mBrush != Brush::Bucket);

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

                    if (Slot = Ground->Claim(Paint); Slot == Splatmap::kSlots)
                    {
                        LOG_W("Ground: region {0},{1} already blends four terrains", Region.GetX(), Region.GetY());
                        return;
                    }
                }

                Ground->Blend(
                    static_cast<UInt8>(X - OriginX), static_cast<UInt8>(Y - OriginY), Slot, Strength);
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
            Ground->Settle();
        }
    }
}