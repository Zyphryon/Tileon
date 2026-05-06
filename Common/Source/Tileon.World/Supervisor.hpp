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
#include "Component/Spatial/Bounds.hpp"
#include <Zyphryon.Content/Service.hpp>
#include <Zyphryon.Scene/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Manages regions and cells in the world, providing spatial queries and entity management
    class Supervisor final : public Locator<Scene::Service, Content::Service>
    {
        friend class World;

    public:

        /// \brief Default filename format for storing region data.
        static constexpr ConstStr8 kRegionFilename       = "Resources://World/{}_{}.region";

        /// \brief Extent of the cell hierarchy for loose spatial partitioning (in tiles).
        static constexpr UInt32    kHierarchyLooseExtent = 8;

        /// \brief Extent of the cell hierarchy for tight spatial partitioning (in tiles).
        static constexpr UInt32    kHierarchyTightExtent = 4;

    public:

        /// \brief Constructs a supervisor instance with the specified service host.
        ///
        /// \param Host The service host to associate with the supervisor.
        explicit Supervisor(Ref<Service::Host> Host);

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
        ZYPHRYON_INLINE void QueryEach(IntRect Hitbox, AnyRef<Function> Callback)
        {
            ForEachEntity(Hitbox, Callback);
        }

        /// \brief Checks if any entity intersects the specified hitbox satisfies a given condition.
        ///
        /// \param Hitbox    The area to query for intersecting entities.
        /// \param Predicate The condition function to apply to each entity that intersects the hitbox.
        /// \return `true` if any entity satisfies the condition, `false` otherwise.
        template<typename Function>
        ZYPHRYON_INLINE Bool QueryAnyOf(IntRect Hitbox, AnyRef<Function> Predicate)
        {
            return AnyOfEntity(Hitbox, Predicate);
        }

        /// \brief Queries the frontmost entity within the specified hitbox that intersects it.
        ///
        /// \param Hitbox The area to query for intersecting entities.
        /// \return The frontmost entity that intersects the hitbox, or an empty entity if none are found.
        ZYPHRYON_INLINE Scene::Entity QueryFrontmost(IntRect Hitbox)
        {
            Scene::Entity Result;

            // Find the entity with the highest maximum Y-coordinate that intersects the hitbox and satisfies the filter.
            SInt32 MaxY = INT32_MIN;

            ForEachEntity(Hitbox, [&](Scene::Entity Actor)
            {
                const IntRect AABB = Actor.Get<Bounds>().GetRect();

                if (const SInt32 ActorMaxY = AABB.GetMaximumY(); ActorMaxY > MaxY)
                {
                    if (AABB.Test(Hitbox))
                    {
                        MaxY = ActorMaxY;
                        Result = Actor;
                    }
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
        ZYPHRYON_INLINE static constexpr UInt32 GetKey(SInt32 X, SInt32 Y, IntRect Boundaries)
        {
            return Index2D<UInt32>(X - Boundaries.GetMinimumX(), Y - Boundaries.GetMinimumY(), Boundaries.GetWidth());
        }

        /// \brief Represents a cell in a loose spatial hierarchy, managing entities and their boundaries.
        struct HierarchyLooseCell final
        {
            /// \brief Indicates if the cell's boundaries need to be refreshed.
            Bool        Dirty;

            /// \brief The boundaries of the cell within the grid.
            IntRect     Boundaries;

            /// \brief Mutex for synchronizing access to the hierarchy.
            Mutex       Mutex;

            /// \brief Flat array of cells in the region.
            Set<UInt64> Entities;

            /// \brief Refreshes the cell's boundaries based on its entities.
            ///
            /// \param Scene The scene service to access entity data.
            /// \return The previous boundaries before the refresh.
            ZYPHRYON_INLINE IntRect Refresh(Ref<Scene::Service> Scene)
            {
                const IntRect Previous = Boundaries;

                if (Dirty)
                {
                    if (Entities.empty())
                    {
                        Boundaries = IntRect::Zero();
                    }
                    else
                    {
                        IntRect AABB(INT32_MAX, INT32_MAX, INT32_MIN, INT32_MIN);

                        ForEach([&](UInt64 ID)
                        {
                            const Scene::Entity Actor = Scene.GetEntity(ID);
                            AABB = IntRect::Union(AABB, Actor.Get<Bounds>().GetRect());
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
            ZYPHRYON_INLINE Bool Insert(Scene::Entity Actor)
            {
                Guard Guard(Mutex);
                Entities.emplace(Actor.GetID());

                // Mark cell as dirty to recalculate boundaries later.
                const Bool WasDirty = Dirty;
                Dirty = true;
                return WasDirty;
            }

            /// \brief Removes an entity from the cell.
            ///
            /// \param Actor The entity to remove.
            /// \return `true` if the cell was previously dirty, `false` otherwise.
            ZYPHRYON_INLINE Bool Remove(Scene::Entity Actor)
            {
                Guard Guard(Mutex);
                Entities.erase(Actor.GetID());

                // Mark cell as dirty to recalculate boundaries later.
                const Bool WasDirty = Dirty;
                Dirty = true;
                return WasDirty;
            }

            /// \brief Updates an entity from the cell.
            ///
            /// \param Actor The entity to update.
            /// \return `true` if the cell was previously dirty, `false` otherwise.
            ZYPHRYON_INLINE Bool Update(Scene::Entity Actor)
            {
                Guard Guard(Mutex);

                const Bool WasDirty = Dirty;
                Dirty = true;
                return WasDirty;
            }

            /// \brief Iterates over all entities in the cell and applies a callback function.
            ///
            /// \param Action The callback function to apply to each entity.
            template<typename Function>
            ZYPHRYON_INLINE void ForEach(AnyRef<Function> Action) const
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
            ZYPHRYON_INLINE Bool AnyOf(AnyRef<Function> Predicate) const
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
            ZYPHRYON_INLINE Ref<HierarchyLooseCell> operator=(AnyRef<HierarchyLooseCell> Other) noexcept
            {
                Boundaries = Move(Other.Boundaries);
                Entities   = Move(Other.Entities);
                return (* this);
            }
        };

        /// \brief Represents a cell in a tight spatial hierarchy, managing a registry of entities.
        struct HierarchyTightCell final
        {
            /// \brief Indices of loose cells contained within this tight cell.
            Vector<UInt32> Indices;

            /// \brief Default constructor.
            ZYPHRYON_INLINE HierarchyTightCell() = default;

            /// \brief Inserts a loose cell index into the tight cell.
            ///
            /// \param Index The index of the loose cell to insert.
            ZYPHRYON_INLINE void Insert(UInt32 Index)
            {
                Indices.push_back(Index);
            }

            /// \brief Removes a loose cell index from the tight cell.
            ///
            /// \param Index The index of the loose cell to remove.
            ZYPHRYON_INLINE void Remove(UInt32 Index)
            {
                if (const auto Iterator = std::ranges::find(Indices, Index); Iterator != Indices.end())
                {
                    (* Iterator) = Indices.back();
                    Indices.pop_back();
                }
            }

            /// \brief Iterates over all loose cell indices in the tight cell and applies a callback function.
            ///
            /// \param Action The callback function to apply to each loose cell index.
            template<typename Function>
            ZYPHRYON_INLINE void ForEach(AnyRef<Function> Action) const
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
            ZYPHRYON_INLINE Bool AnyOf(AnyRef<Function> Predicate) const
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
            ZYPHRYON_INLINE Ref<HierarchyTightCell> operator=(AnyRef<HierarchyTightCell> Other) noexcept
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
        ZYPHRYON_INLINE void ForEachEntity(IntRect Volume, AnyRef<Function> Action)
        {
            thread_local Set<UInt32> LooseAlreadyProcessed;
            LooseAlreadyProcessed.clear();

            // Iterate all tight cells within the specified volume.
            ForEachTightCell(GetTightCoordinates(Volume), [&](ConstRef<HierarchyTightCell> TightCell)
            {
                TightCell.ForEach([&](UInt32 LooseKey)
                {
                    // Early reject if the loose cell was already processed.
                    if (!LooseAlreadyProcessed.emplace(LooseKey).second)
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
                        Action(GetService<Scene::Service>().GetEntity(ID));
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
        ZYPHRYON_INLINE Bool AnyOfEntity(IntRect Volume, AnyRef<Function> Predicate)
        {
            thread_local Set<UInt32> LooseAlreadyProcessed;
            LooseAlreadyProcessed.clear();

            // Iterate all tight cells within the specified volume.
            return AnyOfTightCell(GetTightCoordinates(Volume), [&](ConstRef<HierarchyTightCell> TightCell)
            {
                return TightCell.AnyOf([&](UInt32 LooseKey)
                {
                    // Early reject if the loose cell was already processed.
                    if (!LooseAlreadyProcessed.emplace(LooseKey).second)
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
                        return Predicate(GetService<Scene::Service>().GetEntity(ID));
                    });
                });
            });
        }

        /// \brief Iterates over all loose cells within the specified volume, invoking a callback for each cell.
        ///
        /// \param Volume   The volume to iterate over.
        /// \param Callback The function invoked once per loose cell.
        template<typename Function>
        ZYPHRYON_INLINE void ForEachTightCell(IntRect Volume, AnyRef<Function> Callback)
        {
            const UInt32 GridWidth = mTightBoundaries.GetWidth();

            const UInt32 GridStart = Index2D<UInt32>(
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
        ZYPHRYON_INLINE Bool AnyOfTightCell(IntRect Volume, AnyRef<Function> Predicate)
        {
            const UInt32 GridWidth = mTightBoundaries.GetWidth();

            const UInt32 GridStart = Index2D<UInt32>(
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
        ZYPHRYON_INLINE IntRect GetTightCoordinates(IntRect Volume)
        {
            return IntRect::Intersection(Coordinate::GetCell<Math::Log(kHierarchyTightExtent)>(Volume), mTightBoundaries);
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

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        IntRect                    mRegionBoundaries;
        Vector<Scene::Entity>      mRegionList;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        IntRect                    mLooseBoundaries;
        IntRect                    mTightBoundaries;
        Mutex                      mLooseMutex;
        Vector<HierarchyLooseCell> mLooseRegistry;
        Vector<UInt32>             mLooseDirty;
        Vector<HierarchyTightCell> mTightRegistry;
    };
}