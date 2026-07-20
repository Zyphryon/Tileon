// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Coordinate.hpp"
#include "Component.hpp"
#include <Zyphryon.Content/Service.hpp>
#include <Zyphryon.Scene/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Manages regions and cells in the world, providing spatial queries and entity management
    class Supervisor final : public Engine::Locator<Job::Service, Scene::Service, Content::Service>
    {
        friend class World;

    public:

        /// \brief Default filename format for storing region data.
        static constexpr Symbol kRegionFilename       = "Resources://World/{0}_{1}.region";

        /// \brief Extent of the cell hierarchy for loose spatial partitioning (in tiles).
        static constexpr UInt32 kHierarchyLooseExtent = 8;

        /// \brief Extent of the cell hierarchy for tight spatial partitioning (in tiles).
        static constexpr UInt32 kHierarchyTightExtent = 4;

    public:

        /// \brief Constructs a supervisor instance with the specified service host.
        ///
        /// \param Host The service host to associate with the supervisor.
        explicit Supervisor(Ref<Engine::Subsystem::Host> Host);

        /// \brief Tears down releasing resources.
        void Teardown();

        /// \brief Saves the current state of the Supervisor.
        void Save();

        /// \brief Navigates within the specified boundaries.
        ///
        /// \param Boundaries The boundaries to navigate within.
        /// \return `true` if navigation was successful, `false` otherwise.
        Bool Navigate(IntRect Boundaries);

        /// \brief Gets a region entity by its coordinates.
        ///
        /// \param RegionX The X-coordinate of the region.
        /// \param RegionY The Y-coordinate of the region.
        /// \return The region entity corresponding to the given coordinates.
        Scene::Entity GetRegion(SInt16 RegionX, SInt16 RegionY) const;

        /// \brief Gets or loads a region entity by its coordinates.
        ///
        /// \param RegionX         The X-coordinate of the region.
        /// \param RegionY         The Y-coordinate of the region.
        /// \param CreateIfMissing Whether to create the region if it does not exist.
        /// \return The region entity corresponding to the given coordinates.
        Scene::Entity GetOrLoadRegion(SInt16 RegionX, SInt16 RegionY, Bool CreateIfMissing);

        /// \brief Loads a region entity by its coordinates.
        ///
        /// \param RegionX         The X-coordinate of the region.
        /// \param RegionY         The Y-coordinate of the region.
        /// \param CreateIfMissing Whether to create the region if it does not exist.
        /// \return The loaded region entity.
        Scene::Entity LoadRegion(SInt16 RegionX, SInt16 RegionY, Bool CreateIfMissing);

        /// \brief Saves the specified region entity.
        ///
        /// \param Actor The region entity to save.
        void SaveRegion(Scene::Entity Actor);

        /// \brief Queries over entities located within the spatial nodes intersecting the specified hitbox.
        ///
        /// \param Hitbox   The area to query for intersecting entities.
        /// \param Callback The callback function to apply to each entity that intersects the hitbox.
        template<typename Function>
        ZY_INLINE void QueryEach(IntRect Hitbox, AnyRef<Function> Callback)
        {
            ForEachEntity(Hitbox, Callback);
        }

        /// \brief Checks if any entity intersects the specified hitbox satisfies a given condition.
        ///
        /// \param Hitbox    The area to query for intersecting entities.
        /// \param Predicate The condition function to apply to each entity that intersects the hitbox.
        /// \return `true` if any entity satisfies the condition, `false` otherwise.
        template<typename Function>
        ZY_INLINE Bool QueryAnyOf(IntRect Hitbox, AnyRef<Function> Predicate)
        {
            return AnyOfEntity(Hitbox, Predicate);
        }

        /// \brief Queries the entity within the specified hitbox that the cursor most likely intends to pick.
        ///
        /// \param Hitbox The area to query for intersecting entities.
        /// \param Point  The precise world-space point the cursor is targeting, in tile units.
        /// \param Band   The distance band within which two origins count as equally close.
        /// \param Bias   The Footprint ratio (0..1) below which a smaller entity outranks a larger one outright.
        /// \return The best-matching entity that intersects the hitbox, or an empty entity if none are found.
        ZY_INLINE Scene::Entity QueryFrontmost(IntRect Hitbox, Vector2 Point, Real32 Band = 0.25f, Real32 Bias = 0.75f)
        {
            Scene::Entity Result;
            Real32        BestDistance = 0.0f;
            SInt32        BestFront    = INT32_MIN;
            Real32        BestArea     = 0.0f;

            ForEachEntity(Hitbox, [&](Scene::Entity Actor)
            {
                const IntRect AABB = Actor.Get<Bound>().GetRect();

                // Skip anything flagged unpickable.
                if (!AABB.Test(Hitbox) || Actor.Has<Unpickable>())
                {
                    return;
                }

                Vector2 Origin;

                if (const ConstPtr<Transform> Transform = Actor.TryGet<const Tileon::Transform>())
                {
                    Origin = Transform->GetWorldspace().GetTranslation() + Vector2(Transform->GetOrigin());
                }
                else
                {
                    Origin = Vector2(
                        AABB.GetMinimumX() + AABB.GetMaximumX(),
                        AABB.GetMinimumY() + AABB.GetMaximumY()) * 0.5f;
                }

                const Real32 Distance = Point.GetDistanceSquared(Origin);
                const SInt32 Front    = AABB.GetMaximumY();
                const Real32 Area     = static_cast<Real32>(AABB.GetArea());

                Bool Better;

                if (!Result.IsValid())
                {
                    Better = true;
                }
                else if (Area < BestArea * Bias)        // meaningfully smaller -> prefer it
                {
                    Better = true;
                }
                else if (BestArea < Area * Bias)        // meaningfully larger -> keep the smaller one
                {
                    Better = false;
                }
                else                                    // comparable size -> closest origin, then frontmost
                {
                    const Bool Closer = Distance < BestDistance - Band;
                    Better = Closer || (Abs(Distance - BestDistance) <= Band && Front > BestFront);
                }

                if (Better)
                {
                    Result       = Actor;
                    BestDistance = Distance;
                    BestFront    = Front;
                    BestArea     = Area;
                }
            });
            return Result;
        }

        /// \brief Updates the hierarchy, refreshing dirty cells and recalculating boundaries as needed.
        void UpdateHierarchy();

    private:

        /// \brief Computes a unique key for a cell based on its coordinates and boundaries.
        ///
        /// \param X          The X-coordinate of the cell.
        /// \param Y          The Y-coordinate of the cell.
        /// \param Boundaries The boundaries of the grid.
        /// \return The unique key representing the cell's position within the grid.
        ZY_INLINE static constexpr UInt32 GetKey(SInt32 X, SInt32 Y, IntRect Boundaries)
        {
            return ConvertTo1D<UInt32>(X - Boundaries.GetMinimumX(), Y - Boundaries.GetMinimumY(), Boundaries.GetWidth());
        }

        /// \brief Represents a cell in a loose spatial hierarchy, managing entities and their boundaries.
        struct HierarchyLooseCell final
        {
            /// \brief Indicates if the cell's boundaries need to be refreshed.
            Atomic<Bool> Dirty;

            /// \brief The boundaries of the cell within the grid.
            IntRect      Boundaries;

            /// \brief Flat array of cells in the region.
            Bag<UInt64>  Entities;

            /// \brief Mutex for synchronizing access to the hierarchy.
            Mutex        Mutex;

            /// \brief Default constructor.
            ZY_INLINE HierarchyLooseCell()
                : Dirty { false }
            {
            }

            /// \brief Move constructor.
            ZY_INLINE HierarchyLooseCell(AnyRef<HierarchyLooseCell> Other) noexcept
                : Dirty      { Other.Dirty.load() },
                  Boundaries { Other.Boundaries },
                  Entities   { Move(Other.Entities) }
            {
            }

            /// \brief Refreshes the cell's boundaries based on its entities.
            ///
            /// \param Scene The scene service to access entity data.
            /// \return The previous boundaries before the refresh.
            ZY_INLINE IntRect Refresh(Ref<Scene::Service> Scene)
            {
                const IntRect Previous = Boundaries;

                if (Dirty)
                {
                    if (Entities.IsEmpty())
                    {
                        Boundaries = IntRect::Zero();
                    }
                    else
                    {
                        IntRect AABB(INT32_MAX, INT32_MAX, INT32_MIN, INT32_MIN);

                        ForEach([&](UInt64 ID)
                        {
                            const Scene::Entity Actor = Scene.GetEntity(ID);
                            ZY_ASSERT(Actor.IsValid(), "Loose cell contains a dangling entity id");

                            AABB = IntRect::Union(AABB, Actor.Get<Bound>().GetRect());
                        });
                        Boundaries = AABB;
                    }
                    Dirty = false;
                }
                return Previous;
            }

            /// \brief Inserts an entity into the cell.
            ///
            /// \param Actor The entity to insert.
            /// \return `true` if the cell was previously dirty, `false` otherwise.
            ZY_INLINE Bool Insert(Scene::Entity Actor)
            {
                Guard Guard(Mutex);
                Entities.Insert(Actor.GetID());

                // Mark cell as dirty to recalculate boundaries later.
                return Dirty.exchange(true);
            }

            /// \brief Removes an entity from the cell.
            ///
            /// \param Actor The entity to remove.
            /// \return `true` if the cell was previously dirty, `false` otherwise.
            ZY_INLINE Bool Remove(Scene::Entity Actor)
            {
                Guard Guard(Mutex);
                Entities.Erase(Actor.GetID());
                return Dirty.exchange(true);
            }

            /// \brief Flags the cell for a boundary refresh after one of its entities moved within it.
            ///
            /// \return `true` if the cell was previously dirty, `false` otherwise.
            ZY_INLINE Bool Update()
            {
                // Membership is unchanged, so only the flag moves and no lock is required.
                return Dirty.exchange(true);
            }

            /// \brief Iterates over all entities in the cell and applies a callback function.
            ///
            /// \param Action The callback function to apply to each entity.
            template<typename Function>
            ZY_INLINE void ForEach(AnyRef<Function> Action) const
            {
                for (const UInt64 ID : Entities)
                {
                    Action(ID);
                }
            }

            /// \brief Checks if any entity in the cell satisfies a given condition.
            ///
            /// \param Predicate The condition function to apply to each entity.
            /// \return `true` if any entity satisfies the condition, `false` otherwise.
            template<typename Function>
            ZY_INLINE Bool AnyOf(AnyRef<Function> Predicate) const
            {
                for (const UInt64 ID : Entities)
                {
                    if (Predicate(ID))
                    {
                        return true;
                    }
                }
                return false;
            }

            /// \brief Move assignment operator.
            ZY_INLINE Ref<HierarchyLooseCell> operator=(AnyRef<HierarchyLooseCell> Other) noexcept
            {
                Dirty.store(Other.Dirty.exchange(false));
                Boundaries = Move(Other.Boundaries);
                Entities   = Move(Other.Entities);
                return (* this);
            }
        };

        /// \brief Represents a cell in a tight spatial hierarchy, managing a registry of entities.
        struct HierarchyTightCell final
        {
            /// \brief Indices of loose cells contained within this tight cell.
            Sequence<UInt32> Indices;

            /// \brief Default constructor.
            ZY_INLINE HierarchyTightCell() = default;

            /// \brief Move constructor.
            ZY_INLINE HierarchyTightCell(AnyRef<HierarchyTightCell> Other) noexcept
                : Indices { Move(Other.Indices) }
            {
            }

            /// \brief Inserts a loose cell index into the tight cell.
            ///
            /// \param Index The index of the loose cell to insert.
            ZY_INLINE void Insert(UInt32 Index)
            {
                Indices.Append(Index);
            }

            /// \brief Removes a loose cell index from the tight cell.
            ///
            /// \param Index The index of the loose cell to remove.
            ZY_INLINE void Remove(UInt32 Index)
            {
                Indices.RemoveFastIf([Index](UInt32 Cell)
                {
                    return Index == Cell;
                });
            }

            /// \brief Iterates over all loose cell indices in the tight cell and applies a callback function.
            ///
            /// \param Action The callback function to apply to each loose cell index.
            template<typename Function>
            ZY_INLINE void ForEach(AnyRef<Function> Action) const
            {
                for (const UInt32 Index : Indices)
                {
                    Action(Index);
                }
            }

            /// \brief Checks if any loose cell index in the tight cell satisfies a given condition.
            ///
            /// \param Predicate The condition function to apply to each loose cell index.
            /// \return `true` if any index satisfies the condition, `false` otherwise.
            template<typename Function>
            ZY_INLINE Bool AnyOf(AnyRef<Function> Predicate) const
            {
                for (const UInt32 Index : Indices)
                {
                    if (Predicate(Index))
                    {
                        return true;
                    }
                }
                return false;
            }

            /// \brief Move assignment operator.
            ZY_INLINE Ref<HierarchyTightCell> operator=(AnyRef<HierarchyTightCell> Other) noexcept
            {
                Indices = Move(Other.Indices);
                return (* this);
            }
        };

        /// \brief Adjusts the hierarchy to fit within the specified boundaries.
        ///
        /// \param Boundaries The new boundaries to adjust the hierarchy to.
        void AdjustHierarchy(IntRect Boundaries);

        /// \brief Iterates over entities located within the spatial nodes intersecting the specified hitbox.
        ///
        /// \param Volume The area to query for intersecting entities.
        /// \param Action The function invoked for each candidate entity.
        template<typename Function>
        ZY_INLINE void ForEachEntity(IntRect Volume, AnyRef<Function> Action)
        {
            Ref<Scene::Service> Scene = GetService<Scene::Service>();

            thread_local Bag<UInt32> LooseAlreadyProcessed;
            LooseAlreadyProcessed.Clear();

            // Iterate all tight cells within the specified volume.
            ForEachTightCell(GetTightCoordinates(Volume), [&](ConstRef<HierarchyTightCell> TightCell)
            {
                TightCell.ForEach([&](UInt32 LooseKey)
                {
                    // Early reject if the loose cell was already processed.
                    if (!LooseAlreadyProcessed.Insert(LooseKey))
                    {
                        return;
                    }

                    ConstRef<HierarchyLooseCell> LooseCell = mLooseRegistry[LooseKey];

                    // Early reject if the loose cell AABB does not intersect the query.
                    if (IntRect::Intersection(Volume, LooseCell.Boundaries).IsAlmostZero())
                    {
                        return;
                    }

                    // Iterate all entities inside the loose cell.
                    LooseCell.ForEach([&](UInt64 ID)
                    {
                        Action(Scene.GetEntity(ID));
                    });
                });
            });
        }

        /// \brief Checks if any entity intersects the specified hitbox satisfies a given condition.
        ///
        /// \param Volume    The area to query for intersecting entities.
        /// \param Predicate The condition function to apply to each entity that intersects the hitbox.
        /// \return `true` if any entity satisfies the condition, `false` otherwise.
        template<typename Function>
        ZY_INLINE Bool AnyOfEntity(IntRect Volume, AnyRef<Function> Predicate)
        {
            Ref<Scene::Service> Scene = GetService<Scene::Service>();

            thread_local Bag<UInt32> LooseAlreadyProcessed;
            LooseAlreadyProcessed.Clear();

            // Iterate all tight cells within the specified volume.
            return AnyOfTightCell(GetTightCoordinates(Volume), [&](ConstRef<HierarchyTightCell> TightCell)
            {
                return TightCell.AnyOf([&](UInt32 LooseKey)
                {
                    // Early reject if the loose cell was already processed.
                    if (!LooseAlreadyProcessed.Insert(LooseKey))
                    {
                        return false;
                    }

                    ConstRef<HierarchyLooseCell> LooseCell = mLooseRegistry[LooseKey];

                    // Early reject if the loose cell AABB does not intersect the query.
                    if (IntRect::Intersection(Volume, LooseCell.Boundaries).IsAlmostZero())
                    {
                        return false;
                    }

                    // Iterate all entities inside the loose cell.
                    return LooseCell.AnyOf([&](UInt64 ID)
                    {
                        return Predicate(Scene.GetEntity(ID));
                    });
                });
            });
        }

        /// \brief Iterates over all loose cells within the specified volume, invoking a callback for each cell.
        ///
        /// \param Volume   The volume to iterate over.
        /// \param Callback The function invoked once per loose cell.
        template<typename Function>
        ZY_INLINE void ForEachTightCell(IntRect Volume, AnyRef<Function> Callback)
        {
            const UInt32 GridWidth = mTightBoundaries.GetWidth();

            const UInt32 GridStart = ConvertTo1D<UInt32>(
                Volume.GetMinimumX() - mTightBoundaries.GetMinimumX(),
                Volume.GetMinimumY() - mTightBoundaries.GetMinimumY(),
                GridWidth);
            const UInt32 GridEnd   = GridStart + (Volume.GetHeight() * GridWidth);

            for (UInt32 RowStart = GridStart; RowStart < GridEnd; RowStart += GridWidth)
            {
                const UInt32 RowEnd = RowStart + Volume.GetWidth();

                for (UInt32 CurrentRow = RowStart; CurrentRow < RowEnd; ++CurrentRow)
                {
                    Callback(mTightRegistry[CurrentRow]);
                }
            }
        }

        /// \brief Checks if any loose cell within the specified volume satisfies a given condition.
        ///
        /// \param Volume    The volume to check within.
        /// \param Predicate The condition function to apply to each loose cell.
        /// \return `true` if any loose cell satisfies the condition, `false` otherwise
        template<typename Function>
        ZY_INLINE Bool AnyOfTightCell(IntRect Volume, AnyRef<Function> Predicate)
        {
            const UInt32 GridWidth = mTightBoundaries.GetWidth();

            const UInt32 GridStart = ConvertTo1D<UInt32>(
                Volume.GetMinimumX() - mTightBoundaries.GetMinimumX(),
                Volume.GetMinimumY() - mTightBoundaries.GetMinimumY(),
                GridWidth);
            const UInt32 GridEnd   = GridStart + (Volume.GetHeight() * GridWidth);

            for (UInt32 RowStart = GridStart; RowStart < GridEnd; RowStart += GridWidth)
            {
                const UInt32 RowEnd = RowStart + Volume.GetWidth();

                for (UInt32 CurrentRow = RowStart; CurrentRow < RowEnd; ++CurrentRow)
                {
                    if (Predicate(mTightRegistry[CurrentRow]))
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        /// \brief Gets the tight cell coordinates for a given volume.
        ///
        /// \param Volume The volume to get tight cell coordinates for.
        /// \return The tight cell coordinates.
        ZY_INLINE IntRect GetTightCoordinates(IntRect Volume)
        {
            return IntRect::Intersection(Coordinate::GetCell<Base::Log(kHierarchyTightExtent)>(Volume), mTightBoundaries);
        }

        /// \brief Inserts an entity into the appropriate cell based on its volume.
        ///
        /// \param Actor  The entity to insert.
        /// \param Center The center coordinates of the entity's volume.
        void InsertEntityOnCell(Scene::Entity Actor, IntVector2 Center);

        /// \brief Removes an entity from the appropriate cell based on its volume.
        ///
        /// \param Actor  The entity to remove.
        /// \param Center The center coordinates of the entity's volume.
        void RemoveEntityOnCell(Scene::Entity Actor, IntVector2 Center);

        /// \brief Updates an entity's position in the grid based on its old and new volumes.
        ///
        /// \param Actor        The entity to update.
        /// \param OldestCenter The previous center coordinates of the entity's volume before the update.
        /// \param NewestCenter The new center coordinates of the entity's volume after the update.
        void UpdateEntityOnCell(Scene::Entity Actor, IntVector2 OldestCenter, IntVector2 NewestCenter);

        /// \brief Recursively unlinks an entity and all of its descendants from their spatial cells.
        ///
        /// \param Root The subtree root to detach; call this before destroying the subtree.
        void DetachEntityOnCell(Scene::Entity Root);

        /// \brief Handles asynchronous load result of a \ref Region.
        ///
        /// \param Result         The result of the asynchronous load operation.
        /// \param File            The loaded file blob.
        /// \param Handle          The handle of the entity associated with the load operation.
        /// \param RegionX         The X coordinate of the region.
        /// \param RegionY         The Y coordinate of the region.
        /// \param CreateIfMissing Whether to create the region if it is missing.
        void OnAsyncLoad(Filesystem::Result Result, Blob File, UInt64 Handle, SInt16 RegionX, SInt16 RegionY, Bool CreateIfMissing);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        IntRect                      mRegionBoundaries;
        Sequence<Scene::Entity>      mRegionList;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        IntRect                      mLooseBoundaries;
        IntRect                      mTightBoundaries;
        Mutex                        mLooseMutex;
        Sequence<HierarchyLooseCell> mLooseRegistry;
        Sequence<UInt32>             mLooseDirty;
        Sequence<HierarchyTightCell> mTightRegistry;
    };
}