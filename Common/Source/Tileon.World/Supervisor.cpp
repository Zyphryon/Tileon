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

#include "Supervisor.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Supervisor::Supervisor(Ref<Engine::Subsystem::Host> Host)
        : Locator { Host }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::Teardown()
    {
        for (Scene::Entity Actor : mRegionList)
        {
            if (Actor.IsValid())
            {
                Actor.Destruct();
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::Save()
    {
        for (Scene::Entity Actor : mRegionList)
        {
            if (Actor.IsValid() && Actor.Has<Persist>())
            {
                SaveRegion(Actor);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Supervisor::Navigate(IntRect Boundaries)
    {
        if (mRegionBoundaries == Boundaries)
        {
            return false;
        }

        Sequence<Scene::Entity> Registry(Boundaries.GetWidth() * Boundaries.GetHeight());
        Registry.Advance(Registry.GetCapacity());

        // Dispose regions that fall outside new boundaries.
        IntRect::ForEachRectDiff(mRegionBoundaries, Boundaries, [&](IntRect Difference)
        {
            for (SInt32 Y = Difference.GetMinimumY(); Y < Difference.GetMaximumY(); ++Y)
            {
                for (SInt32 X = Difference.GetMinimumX(); X < Difference.GetMaximumX(); ++X)
                {
                    Scene::Entity Actor = mRegionList[GetKey(X, Y, mRegionBoundaries)];

                    if (Actor.IsValid())
                    {
                        if (Actor.Has<Persist>())
                        {
                            SaveRegion(Actor);
                        }

                        DetachEntityOnCell(Actor);
                        Actor.Destruct();
                    }
                }
            }
        });

        // Load regions newly included in boundaries.
        IntRect::ForEachRectDiff(Boundaries, mRegionBoundaries, [&](IntRect Difference)
        {
            for (SInt32 Y = Difference.GetMinimumY(); Y < Difference.GetMaximumY(); ++Y)
            {
                for (SInt32 X = Difference.GetMinimumX(); X < Difference.GetMaximumX(); ++X)
                {
                    Registry[GetKey(X, Y, Boundaries)] = LoadRegion(X, Y, false);
                }
            }
        });

        // Preserve regions that overlap old and new boundaries.
        const IntRect Intersect = IntRect::Intersection(mRegionBoundaries, Boundaries);

        for (SInt32 Y = Intersect.GetMinimumY(); Y < Intersect.GetMaximumY(); ++Y)
        {
            for (SInt32 X = Intersect.GetMinimumX(); X < Intersect.GetMaximumX(); ++X)
            {
                Registry[GetKey(X, Y, Boundaries)] = Move(mRegionList[GetKey(X, Y, mRegionBoundaries)]);
            }
        }

        mRegionList       = Move(Registry);
        mRegionBoundaries = Boundaries;

        // Adjust spatial hierarchy to new boundaries.
        AdjustHierarchy(Boundaries * IntVector2(Region::kTilesPerX, Region::kTilesPerY));
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Entity Supervisor::GetRegion(SInt16 RegionX, SInt16 RegionY) const
    {
        Scene::Entity Actor;

        if (const UInt32 Key = GetKey(RegionX, RegionY, mRegionBoundaries); Key < mRegionList.GetSize())
        {
            Actor = mRegionList[Key];
        }
        return Actor;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Entity Supervisor::GetOrLoadRegion(SInt16 RegionX, SInt16 RegionY, Bool CreateIfMissing)
    {
        Scene::Entity Actor = GetRegion(RegionX, RegionY);

        if (!Actor.IsValid())
        {
            Actor = LoadRegion(RegionX, RegionY, CreateIfMissing);

            if (Actor.IsValid() && mRegionBoundaries.Contains(RegionX, RegionY))
            {
                const UInt32 Key = GetKey(RegionX, RegionY, mRegionBoundaries);
                mRegionList[Key] = Actor;
            }
        }
        return Actor;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Entity Supervisor::LoadRegion(SInt16 RegionX, SInt16 RegionY, Bool CreateIfMissing)
    {
        const Str32 Name = Str32::Print<"Region {0}.{1}">(RegionX, RegionY);

        if (Scene::Entity Actor = GetService<Scene::Service>().GetEntity(Name); Actor.IsValid())
        {
            return Actor;
        }
        else
        {
            LOG_D("Supervisor: Loading ({0} {1})", RegionX, RegionY);

            Actor = GetService<Scene::Service>().CreateEntity();
            Actor.SetName(Name);
            Actor.SetAlias(Name);
            Actor.Emplace<Transform>(IntVector3(RegionX * Region::kTilesPerX, 0.0f, RegionY * Region::kTilesPerY));

            const auto OnResult = [this, Handle = Actor.GetID(), RegionX, RegionY, CreateIfMissing](Filesystem::Result Result, Blob File)
            {
                OnAsyncLoad(Result, Move(File), Handle, RegionX, RegionY, CreateIfMissing);
            };
            GetService<Content::Service>().Read(Str::Print<kRegionFilename>(RegionX, RegionY), OnResult);

            return Actor;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::SaveRegion(Scene::Entity Actor)
    {
        ConstRef<Region> Region = Actor.Get<const Tileon::Region>();

        LOG_D("Supervisor: Saving {0} {1}", Region.GetX(), Region.GetY());

        Writer Output;
        GetService<Scene::Service>().SaveHierarchy(Output, Actor);

        Str Filename = Str::Print<kRegionFilename>(Region.GetX(), Region.GetY());
        GetService<Content::Service>().Write(Move(Filename), Output.Detach(), { });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Entity Supervisor::Migrate(Scene::Entity Actor, Ref<Pose> Pose)
    {
        const Scene::Entity Owner = Actor.GetParent();

        if (!Owner.IsValid())
        {
            return Scene::Entity();
        }

        const ConstPtr<Region> Origin = Owner.TryGet<const Tileon::Region>();

        if (!Origin)
        {
            return Scene::Entity();
        }

        // A pose is local to its region, so anything outside the region's own span belongs to a neighbour.
        const Vector3 Position = Pose.GetTranslation();
        const Vector2 Ground   = Position.GetXZ();
        const Vector2 Distance = Vector2(
            Floor(Ground.GetX() / static_cast<Real32>(Region::kTilesPerX)),
            Floor(Ground.GetY() / static_cast<Real32>(Region::kTilesPerY)));

        if (Distance.IsAlmostZero())
        {
            return Scene::Entity();
        }

        const Scene::Entity Target = GetRegion(
            static_cast<SInt16>(Origin->GetX() + static_cast<SInt32>(Distance.GetX())),
            static_cast<SInt16>(Origin->GetY() + static_cast<SInt32>(Distance.GetY())));

        if (!Target.IsValid())
        {
            return Scene::Entity();
        }

        // Rebase the pose before re-parenting: outside a deferred scope the attach moves the entity to another
        // table straight away, which would leave the reference pointing at the row it just vacated.
        Pose.SetTranslation(Vector3(
            Position.GetX() - Distance.GetX() * static_cast<Real32>(Region::kTilesPerX),
            Position.GetY(),
            Position.GetZ() - Distance.GetY() * static_cast<Real32>(Region::kTilesPerY)));

        Actor.Attach(Target, Scene::Hierarchy::Open);

        // Cascades Stale to all static children, making them Kinetic this frame.
        Actor.Add<Stale>();

        return Target;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::AdjustHierarchy(IntRect Boundaries)
    {
        const IntRect NewLooseBoundaries = Coordinate::GetCell<kHierarchyLooseLog>(Boundaries);
        const IntRect NewTightBoundaries = Coordinate::GetCell<kHierarchyTightLog>(Boundaries);

        Sequence<HierarchyLooseCell> NewLooseRegistry(NewLooseBoundaries.GetWidth() * NewLooseBoundaries.GetHeight());
        Sequence<HierarchyTightCell> NewTightRegistry(NewTightBoundaries.GetWidth() * NewTightBoundaries.GetHeight());

        NewLooseRegistry.Advance(NewLooseRegistry.GetCapacity());
        NewTightRegistry.Advance(NewTightRegistry.GetCapacity());

        // Preserve loose cells that overlap old and new boundaries.
        const IntRect LooseIntersect = IntRect::Intersection(mLooseBoundaries, NewLooseBoundaries);

        for (SInt32 Y = LooseIntersect.GetMinimumY(); Y < LooseIntersect.GetMaximumY(); ++Y)
        {
            for (SInt32 X = LooseIntersect.GetMinimumX(); X < LooseIntersect.GetMaximumX(); ++X)
            {
                const UInt32 OldKey = GetKey(X, Y, mLooseBoundaries);
                const UInt32 NewKey = GetKey(X, Y, NewLooseBoundaries);
                NewLooseRegistry[NewKey] = Move(mLooseRegistry[OldKey]);
            }
        }

        // Re-assign all loose cells to tight cells.
        for (SInt32 LooseY = NewLooseBoundaries.GetMinimumY(); LooseY < NewLooseBoundaries.GetMaximumY(); ++LooseY)
        {
            for (SInt32 LooseX = NewLooseBoundaries.GetMinimumX(); LooseX < NewLooseBoundaries.GetMaximumX(); ++LooseX)
            {
                const UInt32 LooseKey = GetKey(LooseX, LooseY, NewLooseBoundaries);
                Ref<HierarchyLooseCell> LooseCell = NewLooseRegistry[LooseKey];

                if (LooseCell.Entities.IsEmpty())
                {
                    continue;
                }

                // Refresh loose cell boundaries based on contained entities.
                LooseCell.Refresh(GetService<Scene::Service>());

                // Calculate which tight cells this loose cell belongs to
                const IntRect TightCoords = IntRect::Intersection(
                    Coordinate::GetCellRange<kHierarchyTightLog>(LooseCell.Boundaries.GetXZ()), NewTightBoundaries);

                for (SInt32 TightY = TightCoords.GetMinimumY(); TightY < TightCoords.GetMaximumY(); ++TightY)
                {
                    for (SInt32 TightX = TightCoords.GetMinimumX(); TightX < TightCoords.GetMaximumX(); ++TightX)
                    {
                        const UInt32 TightKey = GetKey(TightX, TightY, NewTightBoundaries);
                        NewTightRegistry[TightKey].Insert(LooseKey);
                    }
                }
            }
        }

        mLooseBoundaries = NewLooseBoundaries;
        mTightBoundaries = NewTightBoundaries;
        mLooseRegistry   = Move(NewLooseRegistry);
        mTightRegistry   = Move(NewTightRegistry);
        mLooseDirty.Clear();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::UpdateHierarchy()
    {
        Ref<Scene::Service> Scene = GetService<Scene::Service>();

        for (const UInt32 LooseKey : mLooseDirty)
        {
            Ref<HierarchyLooseCell> Loose = mLooseRegistry[LooseKey];

            // Refresh loose cell boundaries based on contained entities.
            const IntBox PreviousBoundaries = Loose.Refresh(Scene);

            if (PreviousBoundaries != Loose.Boundaries)
            {
                const IntRect OldestIntersect = GetTightCoordinates(PreviousBoundaries.GetXZ());

                if (!Loose.Entities.IsEmpty())
                {
                    const IntRect NewestIntersect = GetTightCoordinates(Loose.Boundaries.GetXZ());

                    // Remove from tight cells that are no longer overlapped.
                    IntRect::ForEachRectDiff(OldestIntersect, NewestIntersect, [&](IntRect Difference)
                    {
                        ForEachTightCell(Difference, [LooseKey](Ref<HierarchyTightCell> Tight)
                        {
                            Tight.Remove(LooseKey);
                        });
                    });

                    // Add to tight cells that are newly overlapped  .
                    IntRect::ForEachRectDiff(NewestIntersect, OldestIntersect, [&](IntRect Difference)
                    {
                        ForEachTightCell(Difference, [LooseKey](Ref<HierarchyTightCell> Tight)
                        {
                            Tight.Insert(LooseKey);
                        });
                    });
                }
                else
                {
                    // Remove from all previously overlapped tight cells.
                    ForEachTightCell(OldestIntersect, [LooseKey](Ref<HierarchyTightCell> Tight)
                    {
                        Tight.Remove(LooseKey);
                    });
                }
            }
        }
        mLooseDirty.Clear();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::InsertEntityOnCell(Scene::Entity Actor, Ref<Enclosure> Enclosure, IntVector2 Center)
    {
        const IntVector2 Loose    = GetLooseCoordinate(Center);
        const UInt32     LooseKey = GetKey(Loose.GetX(), Loose.GetY(), mLooseBoundaries);

        // Link entity to loose cell, and remember the cell so it can be unfiled from the same one.
        Enclosure.SetCell(Loose);

        if (!mLooseRegistry[LooseKey].Insert(Actor))
        {
            // Mark cell as dirty for next hierarchy update.
            Guard Guard(mLooseMutex);
            mLooseDirty.Append(LooseKey);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::RemoveEntityOnCell(Scene::Entity Actor, Ref<Enclosure> Enclosure)
    {
        const IntVector2 Loose = Enclosure.GetCell();

        Enclosure.SetCell(Enclosure::kUnlinked);

        if (mLooseBoundaries.Contains(Loose.GetX(), Loose.GetY()))
        {
            const UInt32 LooseKey = GetKey(Loose.GetX(), Loose.GetY(), mLooseBoundaries);

            if (!mLooseRegistry[LooseKey].Remove(Actor))
            {
                // Mark cell as dirty for next hierarchy update.
                Guard Guard(mLooseMutex);
                mLooseDirty.Append(LooseKey);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::UpdateEntityOnCell(Scene::Entity Actor, Ref<Enclosure> Enclosure, IntVector2 NewestCenter)
    {
        const IntVector2 Newest = GetLooseCoordinate(NewestCenter);

        if (Newest == Enclosure.GetCell())
        {
            const UInt32 NewestKey = GetKey(Newest.GetX(), Newest.GetY(), mLooseBoundaries);

            if (!mLooseRegistry[NewestKey].Update())
            {
                // Mark cell as dirty for next hierarchy update.
                Guard Guard(mLooseMutex);
                mLooseDirty.Append(NewestKey);
            }
        }
        else
        {
            RemoveEntityOnCell(Actor, Enclosure);
            InsertEntityOnCell(Actor, Enclosure, NewestCenter);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::DetachEntityOnCell(Scene::Entity Root)
    {
        if (const Ptr<Enclosure> Enclosure = Root.TryGet<Tileon::Enclosure>(); Enclosure && Enclosure->IsLinked())
        {
            RemoveEntityOnCell(Root, * Enclosure);
        }

        Root.Children([this](Scene::Entity Child)
        {
            DetachEntityOnCell(Child);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::OnAsyncLoad(Filesystem::Result Result, Blob File, UInt64 Handle, SInt16 RegionX, SInt16 RegionY, Bool CreateIfMissing)
    {
        if (Result == Filesystem::Result::Success && File)
        {
            GetService<Job::Service>().Dispatch(Job::Lane::Main, [this, Handle, File = Move(File)]
            {
                const Scene::Entity Actor = GetService<Scene::Service>().GetEntity(Handle);

                if (Actor.IsAlive())
                {
                    Reader Input(File.GetData(), File.GetSize());
                    GetService<Scene::Service>().LoadHierarchy(Input, Actor);
                }
            });
        }
        else if (CreateIfMissing)
        {
            GetService<Job::Service>().Dispatch(Job::Lane::Main, [this, Handle, RegionX, RegionY]
            {
                const Scene::Entity Actor = GetService<Scene::Service>().GetEntity(Handle);

                if (Actor.IsAlive())
                {
                    Actor.Emplace<Region>(RegionX, RegionY);
                }
            });
        }
        else
        {
            GetService<Job::Service>().Dispatch(Job::Lane::Main, [this, Handle]
            {
                const Scene::Entity Actor = GetService<Scene::Service>().GetEntity(Handle);

                if (Actor.IsAlive())
                {
                    Actor.Destruct();
                }
            });
        }
    }
}