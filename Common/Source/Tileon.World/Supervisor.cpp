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
#include "Component/Condition/Lifecycle.hpp"
#include "Component/Kinematic/Transform.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Supervisor::Supervisor(Ref<Service::Host> Host)
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
                SaveRegion(Actor);  // TODO: Multithreaden (System?)
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

        Vector<Scene::Entity> Registry(Boundaries.GetWidth() * Boundaries.GetHeight());

        // Dispose regions that fall outside new boundaries.
        IntRect::ForEachRectDiff(mRegionBoundaries, Boundaries, [&](IntRect Difference)
        {
            for (SInt32 Y = Difference.GetMinimumY(); Y < Difference.GetMaximumY(); ++Y)
            {
                for (SInt32 X = Difference.GetMinimumX(); X < Difference.GetMaximumX(); ++X)
                {
                    Scene::Entity Actor = mRegionList[GetKey(X, Y, mRegionBoundaries)];

                    if (Actor.IsValid())   // TODO: Multithreaden (System?)
                    {
                        if (Actor.Has<Persist>())
                        {
                            SaveRegion(Actor);
                        }

                        Actor.Children([this](Scene::Entity Child)
                        {
                            RemoveEntityOnCell(Child, Child.Get<Bounds>().GetRect().GetCenter());
                        });
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
                    Registry[GetKey(X, Y, Boundaries)] = LoadRegion(X, Y, false);   // TODO: Multithreaden (System?)
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
        AdjustHierarchy(Boundaries);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Entity Supervisor::GetRegion(SInt16 RegionX, SInt16 RegionY) const
    {
        Scene::Entity Actor;

        if (const UInt32 Key = GetKey(RegionX, RegionY, mRegionBoundaries); Key < mRegionList.size())
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
        Scene::Entity Actor = GetService<Scene::Service>().GetEntity(Format("Region.{}_{}", RegionX, RegionY));

        if (!Actor.IsValid())
        {
            LOG_DEBUG("Supervisor: Loading ({} {})", RegionX, RegionY);

            const ConstStr8 Filename = Format(kRegionFilename, RegionX, RegionY);

            if (Blob File = GetService<Content::Service>().Find(Content::Uri(Filename)); File)
            {
                Reader Input(File.GetSpan<UInt8>());
                Actor = GetService<Scene::Service>().LoadEntityHierarchy(Input);
            }
            else
            {
                if (CreateIfMissing)
                {
                    Actor = GetService<Scene::Service>().CreateEntity();
                    Actor.SetName(Format("Region.{}_{}", RegionX, RegionY));
                    Actor.Emplace<Region>(RegionX, RegionY);
                }
            }

            if (Actor.IsValid())
            {
                const IntVector2 Origin(RegionX * Region::kTilesPerX, RegionY * Region::kTilesPerY);
                Actor.Emplace<Transform>(Matrix3x2::Identity(), Origin);
            }
        }
        return Actor;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::SaveRegion(Scene::Entity Actor)
    {
        ConstRef<Region> Region = Actor.Get<const Tileon::Region>();

        Writer Output;
        GetService<Scene::Service>().SaveEntityHierarchy(Output, Actor);

        LOG_DEBUG("Supervisor: Saving {} {}", Region.GetX(), Region.GetY());

        const ConstStr8 Filename = Format(kRegionFilename, Region.GetX(), Region.GetY());
        GetService<Content::Service>().Save(Filename, Output.GetData());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::AdjustHierarchy(IntRect Boundaries)
    {
        const IntRect NewLooseBoundaries = Coordinate::GetCell<Math::Log(kHierarchyLooseExtent)>(Boundaries);
        const IntRect NewTightBoundaries = Coordinate::GetCell<Math::Log(kHierarchyTightExtent)>(Boundaries);

        Vector<HierarchyLooseCell> NewLooseRegistry(NewLooseBoundaries.GetWidth() * NewLooseBoundaries.GetHeight());
        Vector<HierarchyTightCell> NewTightRegistry(NewTightBoundaries.GetWidth() * NewTightBoundaries.GetHeight());

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

                if (LooseCell.Entities.empty())
                {
                    continue;
                }

                // Refresh loose cell boundaries based on contained entities.
                LooseCell.Refresh(GetService<Scene::Service>());

                // Calculate which tight cells this loose cell belongs to
                const IntRect TightCoords = IntRect::Intersection(
                    Coordinate::GetCell<Math::Log(kHierarchyTightExtent)>(LooseCell.Boundaries), NewTightBoundaries);

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
            const IntRect PreviousBoundaries = Loose.Refresh(Scene);

            if (PreviousBoundaries != Loose.Boundaries)
            {
                const IntRect OldestIntersect = GetTightCoordinates(PreviousBoundaries);

                if (!Loose.Entities.empty())
                {
                    const IntRect NewestIntersect = GetTightCoordinates(Loose.Boundaries);

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
        mLooseDirty.clear();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::InsertEntityOnCell(Scene::Entity Actor, IntVector2 Center)
    {
        // Link entity to loose cell.
        IntVector2 Loose = Center >> Math::Log(kHierarchyLooseExtent);
        Loose.SetX(Clamp(Loose.GetX(), mLooseBoundaries.GetMinimumX(), mLooseBoundaries.GetMaximumX() - 1));
        Loose.SetY(Clamp(Loose.GetY(), mLooseBoundaries.GetMinimumY(), mLooseBoundaries.GetMaximumY() - 1));

        const UInt32 LooseKey = GetKey(Loose.GetX(), Loose.GetY(), mLooseBoundaries);
        if (!mLooseRegistry[LooseKey].Insert(Actor))
        {
            // Mark cell as dirty for next hierarchy update.
            Guard Guard(mLooseMutex);
            mLooseDirty.emplace_back(LooseKey);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::RemoveEntityOnCell(Scene::Entity Actor, IntVector2 Center)
    {
        // Unlink entity from loose cell.
        IntVector2 Loose    = Center >> Math::Log(kHierarchyLooseExtent);
        Loose.SetX(Clamp(Loose.GetX(), mLooseBoundaries.GetMinimumX(), mLooseBoundaries.GetMaximumX() - 1));
        Loose.SetY(Clamp(Loose.GetY(), mLooseBoundaries.GetMinimumY(), mLooseBoundaries.GetMaximumY() - 1));

        const UInt32 LooseKey = GetKey(Loose.GetX(), Loose.GetY(), mLooseBoundaries);
        if (!mLooseRegistry[LooseKey].Remove(Actor))
        {
            // Mark cell as dirty for next hierarchy update.
            Guard Guard(mLooseMutex);
            mLooseDirty.emplace_back(LooseKey);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Supervisor::UpdateEntityOnCell(Scene::Entity Actor, IntVector2 OldestCenter, IntVector2 NewestCenter)
    {
        IntVector2 OldLoose  = OldestCenter >> Math::Log(kHierarchyLooseExtent);
        OldLoose.SetX(Clamp(OldLoose.GetX(), mLooseBoundaries.GetMinimumX(), mLooseBoundaries.GetMaximumX() - 1));
        OldLoose.SetY(Clamp(OldLoose.GetY(), mLooseBoundaries.GetMinimumY(), mLooseBoundaries.GetMaximumY() - 1));

        IntVector2 NewLoose  = NewestCenter >> Math::Log(kHierarchyLooseExtent);
        NewLoose.SetX(Clamp(NewLoose.GetX(), mLooseBoundaries.GetMinimumX(), mLooseBoundaries.GetMaximumX() - 1));
        NewLoose.SetY(Clamp(NewLoose.GetY(), mLooseBoundaries.GetMinimumY(), mLooseBoundaries.GetMaximumY() - 1));

        if (OldLoose == NewLoose)
        {
            const UInt32 LooseKey = GetKey(NewLoose.GetX(), NewLoose.GetY(), mLooseBoundaries);
            if (!mLooseRegistry[LooseKey].Update(Actor))
            {
                // Mark cell as dirty for next hierarchy update.
                Guard Guard(mLooseMutex);
                mLooseDirty.emplace_back(LooseKey);
            }
        }
        else
        {
            RemoveEntityOnCell(Actor, OldestCenter);
            InsertEntityOnCell(Actor, NewestCenter);
        }
    }
}