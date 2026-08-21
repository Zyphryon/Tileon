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

#include "Entities.hpp"
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

    Entities::Entities(Ref<Context> Context)
        : mContext          { Context },
          mSelectionPrimary { 0 },
          mRevision         { 0 },
          mClipboardCount   { 0 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Entities::Execute(Brush Brush, Command Command, Placement Placement, UInt32 Object)
    {
        switch (Brush)
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

    void Entities::UpdatePreview(Brush Brush, Placement Placement, UInt32 Object)
    {
        if (Brush != Brush::Pencil || Object == 0)
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

    void Entities::ClearPreview()
    {
        if (mPreview.IsAlive())
        {
            mPreview.Add<Dispose>();
        }
        mPreview = Scene::Entity();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Entity Entities::EnsurePreview(Scene::Entity Actor, Scene::Entity Archetype, Placement Placement)
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

    void Entities::AddEntity(Placement Placement, UInt32 Object)
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

    void Entities::RemoveEntity(Placement Placement)
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

    void Entities::SelectEntity(Placement Placement)
    {
        SelectSingle(Placement);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Entities::SetPrimary(UInt64 Entity)
    {
        mSelectionPrimary = Entity;
        mContext.SetInteger("Selection.Entity", static_cast<SInt64>(Entity));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Placement Entities::OriginOf(Scene::Entity Actor) const
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

    void Entities::ReconcileSelection()
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

    void Entities::ClearSelection()
    {
        mSelection.Clear();
        SetPrimary(0);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Entity Entities::ResolveSelection(Placement Placement)
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

    void Entities::SelectSingle(Placement Placement)
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

    void Entities::SelectToggle(Placement Placement)
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

    void Entities::SelectWithin(ConstRef<Camera> Camera, ImVec2 Minimum, ImVec2 Maximum, Bool Additive)
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

    void Entities::DeleteSelection()
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

    void Entities::CopySelection()
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

    void Entities::CutSelection()
    {
        Ref<History> History = mContext.GetHistory();
        History.Open("Cut");

        CopySelection();
        DeleteSelection();

        History.Close();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Entities::Paste(Placement Placement)
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
}