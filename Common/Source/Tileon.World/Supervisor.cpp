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
#include "Placement.hpp"

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
        for (ConstRef<decltype(mRegistry)::Pair> Pair : mRegistry)
        {
            if (const Scene::Entity Actor = Pair.Second->Actor; Actor.IsValid())
            {
                Actor.Destruct();
            }
        }
        mRegistry.Clear();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::Save()
    {
        mRegistry.EraseIf([this](UInt32 Key, Ref<Unique<Slot>> Holder)
        {
            const Scene::Entity Actor = Holder->Actor;

            if (!Actor.IsValid())
            {
                return false;
            }

            if (Actor.Has<Persist>())
            {
                SaveRegion(Actor);

                // Now that it is on disk it is no longer the edit's only copy, so it may be let go again.
                Actor.Remove<Persist>();
            }

            if (mManifest.Contains(Key))
            {
                return false;
            }

            Evict(* Holder);
            return true;
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Supervisor::Reside(ConstSpan<IntVector2> Regions)
    {
        mManifest.Clear();
        mManifest.Reserve(Regions.GetSize());

        for (IntVector2 Coordinate : Regions)
        {
            mManifest.Insert(GetKey(Coordinate.GetX(), Coordinate.GetY()));
        }

        Bool Dirty = false;

        // Retire every resident region the new set no longer asks for.
        mRegistry.EraseIf([&](UInt32 Key, Ref<Unique<Slot>> Holder)
        {
            if (mManifest.Contains(Key))
            {
                return false;
            }

            // An edit lives nowhere but in memory until it is saved, so a region holding one stays resident.
            // Writing it out here instead would put a change on disk that the user never asked to keep.
            if (Holder->Actor.IsValid() && Holder->Actor.Has<Persist>())
            {
                return false;
            }

            Evict(* Holder);
            Dirty = true;
            return true;
        });

        // Bring in every region the new set asks for that is not resident yet.
        for (IntVector2 Coordinate : Regions)
        {
            const UInt32 Key = GetKey(Coordinate.GetX(), Coordinate.GetY());

            if (mRegistry.Contains(Key))
            {
                continue;
            }

            Unique<Slot> Holder = Unique<Slot>::Create();
            Holder->Actor = LoadRegion(Coordinate.GetX(), Coordinate.GetY(), false);

            mRegistry.Assign(Key, Move(Holder));
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Entity Supervisor::GetRegion(SInt16 RegionX, SInt16 RegionY) const
    {
        const ConstPtr<Unique<Slot>> Holder = mRegistry.Find(GetKey(RegionX, RegionY));
        return Holder ? (* Holder)->Actor : Scene::Entity();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Entity Supervisor::GetOrLoadRegion(SInt16 RegionX, SInt16 RegionY, Bool CreateIfMissing)
    {
        const UInt32 Key = GetKey(RegionX, RegionY);

        if (const Ptr<Unique<Slot>> Holder = mRegistry.Find(Key); Holder && (* Holder)->Actor.IsValid())
        {
            return (* Holder)->Actor;
        }

        const Scene::Entity Actor = LoadRegion(RegionX, RegionY, CreateIfMissing);

        // A region loaded on demand becomes resident like any other, so it is saved and evicted with the rest.
        if (Actor.IsValid())
        {
            if (const Ptr<Unique<Slot>> Holder = mRegistry.Find(Key))
            {
                (* Holder)->Actor = Actor;
            }
            else
            {
                Unique<Slot> Fresh = Unique<Slot>::Create();
                Fresh->Actor = Actor;

                mRegistry.Assign(Key, Move(Fresh));
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
            Actor.Emplace<Transform>(IntVector3(RegionX * Region::kUnitsPerX, 0.0f, RegionY * Region::kUnitsPerY));

            const auto OnResult = [this, Handle = Actor.GetID(), RegionX, RegionY, CreateIfMissing](Filesystem::Result Result, Blob File)
            {
                OnAsyncLoad(Result, Move(File), Handle, RegionX, RegionY, CreateIfMissing);
            };
            GetService<Content::Service>().Read(Str::Print<kFilename>(RegionX, RegionY), OnResult);

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

        Str Filename = Str::Print<kFilename>(Region.GetX(), Region.GetY());
        GetService<Content::Service>().Write(Move(Filename), Output.Detach(), { });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Entity Supervisor::Migrate(Scene::Entity Actor, Ref<Pose> Pose)
    {
        const Scene::Entity Source = Actor.GetParent();
        ZY_ASSERT(Source.IsValid(), "A migrating entity must be parented to the region that owns it");

        // A part follows the root it belongs to, so only an entity a region owns outright may move by itself.
        const ConstPtr<Region> Origin = Source.TryGet<const Tileon::Region>();

        if (Origin == nullptr)
        {
            return Scene::Entity();
        }

        // A pose is local to its region, so anything outside the region's own span belongs to a neighbour.
        const Vector3 Position = Pose.GetTranslation();
        const Vector2 Ground   = Position.GetXZ();
        const Vector2 Distance = Vector2::Floor(Ground / Vector2(Region::kUnitsPerX, Region::kUnitsPerY));

        if (Distance.IsAlmostZero())
        {
            return Scene::Entity();
        }

        // The world ends somewhere, so a step past its edge is refused rather than wrapped to the far side.
        const SInt32 TargetX = Clamp(Origin->GetX() + static_cast<SInt32>(Distance.GetX()),
            static_cast<SInt32>(Placement::kMinRegion),
            static_cast<SInt32>(Placement::kMaxRegion));
        const SInt32 TargetY = Clamp(Origin->GetY() + static_cast<SInt32>(Distance.GetY()),
            static_cast<SInt32>(Placement::kMinRegion),
            static_cast<SInt32>(Placement::kMaxRegion));

        const Scene::Entity Target = GetRegion(static_cast<SInt16>(TargetX), static_cast<SInt16>(TargetY));

        if (Target.IsValid())
        {
            // Rebase the pose before re-parenting: outside a deferred scope the attach moves the entity to another
            // table straight away, which would leave the reference pointing at the row it just vacated.
            Pose.SetTranslation(Vector3(
                Position.GetX() - Distance.GetX() * static_cast<Real32>(Region::kUnitsPerX),
                Position.GetY(),
                Position.GetZ() - Distance.GetY() * static_cast<Real32>(Region::kUnitsPerY)));

            // Cascades Stale to all static children, making them Kinetic this frame.
            Actor.Attach(Target, Scene::Hierarchy::Open);
            Actor.Add<Stale>();
        }
        else
        {
            static constexpr Real32 kSpanX = Region::kUnitsPerX * (1.0f - kEpsilon<Real32>);
            static constexpr Real32 kSpanY = Region::kUnitsPerY * (1.0f - kEpsilon<Real32>);

            Pose.SetTranslation(Vector3(
                Clamp(Position.GetX(), 0.0f, kSpanX),
                Position.GetY(),
                Clamp(Position.GetZ(), 0.0f, kSpanY)));
        }
        return Target;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::UpdateHierarchy()
    {
        Ref<Scene::Service> Scene = GetService<Scene::Service>();

        IntVector2 Reach = IntVector2::Zero();

        for (ConstRef<decltype(mRegistry)::Pair> Pair : mRegistry)
        {
            Ref<Slot> Slot = (* Pair.Second);

            // A region no entity touched this frame keeps all of its cells out of the sweep.
            if (Exchange(Slot.Dirty, false))
            {
                IntBox Boundaries(INT32_MAX, INT32_MAX, INT32_MAX, INT32_MIN, INT32_MIN, INT32_MIN);
                Bool   Bounded = false;

                for (UInt32 Local = 0; Local < kLooseCount; ++Local)
                {
                    Ref<HierarchyLooseCell> Loose = Slot.Loose[Local];

                    // Refresh loose cell boundaries based on contained entities.
                    Loose.Refresh(Scene);

                    if (!Loose.Entities.IsEmpty())
                    {
                        Boundaries = IntBox::Union(Boundaries, Loose.Boundaries);
                        Bounded    = true;
                    }
                }

                Slot.Boundaries = Bounded ? Boundaries : IntBox::Zero();
                Slot.Reach      = GetReach(GetKeyCoordinate(Pair.First), Slot.Boundaries);
            }

            // A query widens its walk by the furthest any resident slot spills, the untouched ones included.
            Reach = IntVector2::Max(Reach, Slot.Reach);
        }
        mReach = Reach;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::InsertEntityOnCell(Scene::Entity Actor, Ref<Enclosure> Enclosure, IntVector2 Center)
    {
        const IntVector2 Loose    = GetLooseCoordinate(Center);
        const UInt64     LooseKey = GetLooseKey(Loose);

        // An entity whose centre falls outside the resident set stays unfiled, since no query can reach it there.
        if (const Ptr<Slot> Slot = FindLooseSlot(LooseKey))
        {
            // Link entity to loose cell, and remember the cell so it can be unfiled from the same one.
            Enclosure.SetCell(Loose);

            if (!Slot->Loose[static_cast<UInt32>(LooseKey)].Insert(Actor))
            {
                // Flag the slot so the next hierarchy update sweeps its cells.
                Slot->Dirty = true;
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::RemoveEntityOnCell(Scene::Entity Actor, Ref<Enclosure> Enclosure)
    {
        if (Enclosure.IsLinked())
        {
            const UInt64 LooseKey = GetLooseKey(Enclosure.GetCell());

            Enclosure.SetCell(Enclosure::kUnlinked);

            // The region owning the cell may have been evicted while the entity was still filed under it.
            if (const Ptr<Slot> Slot = FindLooseSlot(LooseKey))
            {
                if (!Slot->Loose[static_cast<UInt32>(LooseKey)].Remove(Actor))
                {
                    // Flag the slot so the next hierarchy update sweeps its cells.
                    Slot->Dirty = true;
                }
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
            const UInt64 LooseKey = GetLooseKey(Newest);

            if (const Ptr<Slot> Slot = FindLooseSlot(LooseKey))
            {
                if (!Slot->Loose[static_cast<UInt32>(LooseKey)].Update())
                {
                    // Flag the slot so the next hierarchy update sweeps its cells.
                    Slot->Dirty = true;
                }
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
        if (const Ptr<Enclosure> Enclosure = Root.TryGet<Tileon::Enclosure>())
        {
            if (Enclosure->IsLinked())
            {
                RemoveEntityOnCell(Root, * Enclosure);
            }
        }

        Root.Children([this](Scene::Entity Child)
        {
            DetachEntityOnCell(Child);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::Evict(Ref<Slot> Slot)
    {
        if (const Scene::Entity Actor = Slot.Actor; Actor.IsValid())
        {
            DetachEntityOnCell(Actor);

            Actor.Destruct();
        }
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