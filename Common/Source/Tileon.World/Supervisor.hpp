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
#include <Zyphryon.Math/Geometry/Ray.hpp>
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

        /// \brief Extent of the cell hierarchy for loose spatial partitioning (in world units).
        static constexpr UInt32 kHierarchyLooseExtent = 8;

        /// \brief Bit shift value for converting world unit coordinates to loose cell coordinates.
        static constexpr UInt32 kHierarchyLooseLog    = Base::Log(kHierarchyLooseExtent);

    public:

        /// \brief Constructs a supervisor instance with the specified service host.
        ///
        /// \param Host The service host to associate with the supervisor.
        explicit Supervisor(Ref<Engine::Subsystem::Host> Host);

        /// \brief Tears down releasing resources.
        void Teardown();

        /// \brief Saves the current state of the Supervisor.
        void Save();

        /// \brief Makes exactly the given regions resident, loading the missing ones and evicting the rest.
        ///
        /// \param Regions The complete set of regions that must be resident once the call returns.
        /// \return `true` if the resident set changed, `false` otherwise.
        Bool Reside(ConstSpan<IntVector2> Regions);

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

        /// \brief Moves an entity into the region its pose now falls in, rebasing the pose to keep it in place.
        ///
        /// \param Actor The entity to migrate, whose parent must be the region it currently belongs to.
        /// \param Pose  The entity's region-local pose, rebased onto the new region when the migration happens.
        /// \return The region the entity moved into, or an invalid entity when it stayed where it was.
        Scene::Entity Migrate(Scene::Entity Actor, Ref<Pose> Pose);

        /// \brief Queries over entities located within the spatial nodes intersecting the specified hitbox.
        ///
        /// \param Hitbox   The area to query for intersecting entities.
        /// \param Callback The callback function to apply to each entity that intersects the hitbox.
        template<typename Function>
        ZY_INLINE void QueryEach(IntBox Hitbox, AnyRef<Function> Callback)
        {
            ForEachEntity(Hitbox, Callback);
        }

        /// \brief Checks if any entity intersects the specified hitbox satisfies a given condition.
        ///
        /// \param Hitbox    The area to query for intersecting entities.
        /// \param Predicate The condition function to apply to each entity that intersects the hitbox.
        /// \return `true` if any entity satisfies the condition, `false` otherwise.
        template<typename Function>
        ZY_INLINE Bool QueryAnyOf(IntBox Hitbox, AnyRef<Function> Predicate)
        {
            return AnyOfEntity(Hitbox, Predicate);
        }

        /// \brief Queries every entity a ray meets, in no particular order.
        ///
        /// \param Ray      The ray to cast, in absolute world coordinates.
        /// \param Limit    The furthest distance along the ray to consider.
        /// \param Callback Invoked with the entity and the distance to it, for every entity the ray meets.
        template<typename Function>
        ZY_INLINE void QueryRay(ConstRef<Ray> Ray, Real32 Limit, AnyRef<Function> Callback)
        {
            const Vector3 Start = Ray.GetOrigin();
            const Vector3 Stop  = Ray.GetPoint(Limit);
            const IntBox  Swept = Box::Enclose<SInt32>(Box(Vector3::Min(Start, Stop), Vector3::Max(Start, Stop)));

            ForEachEntity(Swept, [&](Scene::Entity Actor)
            {
                if (Actor.Has<Unpickable>())
                {
                    return;
                }

                ConstRef<Enclosure> Enclosure = Actor.Get<const Tileon::Enclosure>();

                // The ray cannot leave its own swept volume, so anything clear of it is out before any float work.
                if (const IntBox AABB = Enclosure.GetVolume(); IntBox::Overlaps(Swept, AABB))
                {
                    const Box Volume(Vector3(AABB.GetMinimum()), Vector3(AABB.GetMaximum()));

                    if (Real32 Distance; Ray::Intersects(Ray, Volume, Distance, Limit))
                    {
                        Callback(Actor, Distance);
                    }
                }
            });
        }

        /// \brief Updates the hierarchy, refreshing dirty cells and recalculating boundaries as needed.
        void UpdateHierarchy();

    private:

        /// \brief Bit shift value for converting a loose cell x-coordinate to the slot that owns it.
        static constexpr SInt32 kLooseShiftX = Coordinate::kBitShiftLocalX - static_cast<SInt32>(kHierarchyLooseLog);

        /// \brief Bit shift value for converting a loose cell y-coordinate to the slot that owns it.
        static constexpr SInt32 kLooseShiftY = Coordinate::kBitShiftLocalY - static_cast<SInt32>(kHierarchyLooseLog);

        /// \brief Number of loose cells a slot spans per x-axis.
        static constexpr SInt32 kLooseSizeX  = 1 << kLooseShiftX;

        /// \brief Number of loose cells a slot spans per y-axis.
        static constexpr SInt32 kLooseSizeY  = 1 << kLooseShiftY;

        /// \brief Total number of loose cells a slot holds.
        static constexpr UInt32 kLooseCount  = kLooseSizeX * kLooseSizeY;

        /// \brief Represents a cell in a loose spatial hierarchy, managing entities and their boundaries.
        struct HierarchyLooseCell final
        {
            /// \brief Indicates if the cell's boundaries need to be refreshed.
            Bool        Dirty;

            /// \brief The boundaries of the cell within the grid.
            IntBox      Boundaries;

            /// \brief Flat array of cells in the region.
            Bag<UInt64> Entities;

            /// \brief Default constructor.
            ZY_INLINE HierarchyLooseCell()
                : Dirty { false }
            {
            }

            /// \brief Refreshes the cell's boundaries based on its entities.
            ///
            /// \param Scene The scene service to access entity data.
            ZY_INLINE void Refresh(Ref<Scene::Service> Scene)
            {
                if (Dirty)
                {
                    if (Entities.IsEmpty())
                    {
                        Boundaries = IntBox::Zero();
                    }
                    else
                    {
                        IntBox AABB(INT32_MAX, INT32_MAX, INT32_MAX, INT32_MIN, INT32_MIN, INT32_MIN);
                        Bool   Bounded = false;

                        ForEach([&](UInt64 ID)
                        {
                            const Scene::Entity Actor = Scene.GetEntity(ID);
                            ZY_ASSERT(Actor.IsValid(), "Loose cell contains a dangling entity id");

                            if (Actor.Has<Enclosure>())
                            {
                                AABB    = IntBox::Union(AABB, Actor.Get<Enclosure>().GetVolume());
                                Bounded = true;
                            }
                        });
                        Boundaries = Bounded ? AABB : IntBox::Zero();
                    }
                    Dirty = false;
                }
            }

            /// \brief Inserts an entity into the cell.
            ///
            /// \param Actor The entity to insert.
            /// \return `true` if the cell was previously dirty, `false` otherwise.
            ZY_INLINE Bool Insert(Scene::Entity Actor)
            {
                Entities.Insert(Actor.GetID());

                // Mark cell as dirty to recalculate boundaries later.
                return Exchange(Dirty, true);
            }

            /// \brief Removes an entity from the cell.
            ///
            /// \param Actor The entity to remove.
            /// \return `true` if the cell was previously dirty, `false` otherwise.
            ZY_INLINE Bool Remove(Scene::Entity Actor)
            {
                Entities.Erase(Actor.GetID());

                return Exchange(Dirty, true);
            }

            /// \brief Flags the cell for a boundary refresh after one of its entities moved within it.
            ///
            /// \return `true` if the cell was previously dirty, `false` otherwise.
            ZY_INLINE Bool Update()
            {
                return Exchange(Dirty, true);
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
        };

        /// \brief Holds a resident region together with the slice of the spatial hierarchy that covers it.
        struct Slot final
        {
            /// \brief Indicates that at least one of the slot's cells changed since the last hierarchy update.
            Bool                                   Dirty;

            /// \brief The region entity, which is invalid while its content is still loading.
            Scene::Entity                          Actor;

            /// \brief Union of the slot's loose cell boundaries, which rejects the whole slot in one test.
            IntBox                                 Boundaries;

            /// \brief How far, in regions, the slot's boundaries reach past the region it holds.
            IntVector2                             Reach;

            /// \brief The loose cells covering the region, in row-major order.
            Array<HierarchyLooseCell, kLooseCount> Loose;
        };

        /// \brief Gets the slot a region is resident under.
        ///
        /// \param RegionX The X-coordinate of the region.
        /// \param RegionY The Y-coordinate of the region.
        /// \return The slot holding the region, or `nullptr` if the region is not resident.
        ZY_INLINE Ptr<Slot> FindSlot(SInt32 RegionX, SInt32 RegionY)
        {
            const Ptr<Unique<Slot>> Holder = mRegistry.Find(GetKey(RegionX, RegionY));
            return Holder ? Holder->Grab() : nullptr;
        }

        /// \brief Gets the slot owning the loose cell a key refers to.
        ///
        /// \param LooseKey The key of the loose cell, as built by \ref GetLooseKey.
        /// \return The slot, or `nullptr` if the region owning the cell is not resident.
        ZY_INLINE Ptr<Slot> FindLooseSlot(UInt64 LooseKey)
        {
            const Ptr<Unique<Slot>> Holder = mRegistry.Find(static_cast<UInt32>(LooseKey >> 32u));
            return Holder ? Holder->Grab() : nullptr;
        }

        /// \brief Gets the loose cell a key refers to.
        ///
        /// \param LooseKey The key of the loose cell, as built by \ref GetLooseKey.
        /// \return The cell, or `nullptr` if the region owning it is not resident.
        ZY_INLINE Ptr<HierarchyLooseCell> FindLooseCell(UInt64 LooseKey)
        {
            const Ptr<Slot> Slot = FindLooseSlot(LooseKey);
            return Slot ? AddressOf(Slot->Loose[static_cast<UInt32>(LooseKey)]) : nullptr;
        }

        /// \brief Iterates over entities located within the spatial nodes intersecting the specified hitbox.
        ///
        /// \param Volume The volume to query for intersecting entities.
        /// \param Action The function invoked for each candidate entity.
        template<typename Function>
        ZY_INLINE void ForEachEntity(IntBox Volume, AnyRef<Function> Action)
        {
            Ref<Scene::Service> Scene = GetService<Scene::Service>();

            // Walk the slots the query reaches, then reject per cell on the full volume.
            ForEachSlot(Volume, [&](ConstRef<Slot> Slot)
            {
                for (UInt32 Local = 0; Local < kLooseCount; ++Local)
                {
                    ConstRef<HierarchyLooseCell> LooseCell = Slot.Loose[Local];

                    // Early reject if the loose cell volume does not intersect the query.
                    if (!IntBox::Overlaps(Volume, LooseCell.Boundaries))
                    {
                        continue;
                    }

                    // Iterate all entities inside the loose cell.
                    for (const UInt64 ID : LooseCell.Entities)
                    {
                        Action(Scene.GetEntity(ID));
                    }
                }
                return false;
            });
        }

        /// \brief Checks if any entity intersects the specified hitbox satisfies a given condition.
        ///
        /// \param Volume    The volume to query for intersecting entities.
        /// \param Predicate The condition function to apply to each entity that intersects the hitbox.
        /// \return `true` if any entity satisfies the condition, `false` otherwise.
        template<typename Function>
        ZY_INLINE Bool AnyOfEntity(IntBox Volume, AnyRef<Function> Predicate)
        {
            Ref<Scene::Service> Scene = GetService<Scene::Service>();

            // Walk the slots the query reaches, then reject per cell on the full volume.
            return ForEachSlot(Volume, [&](ConstRef<Slot> Slot)
            {
                for (UInt32 Local = 0; Local < kLooseCount; ++Local)
                {
                    ConstRef<HierarchyLooseCell> LooseCell = Slot.Loose[Local];

                    // Early reject if the loose cell volume does not intersect the query.
                    if (!IntBox::Overlaps(Volume, LooseCell.Boundaries))
                    {
                        continue;
                    }

                    // Iterate all entities inside the loose cell.
                    for (const UInt64 ID : LooseCell.Entities)
                    {
                        if (Predicate(Scene.GetEntity(ID)))
                        {
                            return true;
                        }
                    }
                }
                return false;
            });
        }

        /// \brief Visits every resident slot whose boundaries meet the given volume.
        ///
        /// \param Volume    The volume to walk, in absolute world units.
        /// \param Predicate Invoked per slot that survives the rejection; returning `true` stops the walk.
        /// \return `true` if the predicate stopped the walk, `false` if it ran to completion.
        template<typename Function>
        ZY_INLINE Bool ForEachSlot(IntBox Volume, AnyRef<Function> Predicate)
        {
            const IntRect Ground = Volume.GetXZ();

            // A cell's boundaries may reach past the region holding it, so the walk widens by the furthest reach.
            const SInt32 MinRegionX = (Ground.GetMinimumX() >> Coordinate::kBitShiftLocalX) - mReach.GetX();
            const SInt32 MinRegionY = (Ground.GetMinimumY() >> Coordinate::kBitShiftLocalY) - mReach.GetY();
            const SInt32 MaxRegionX = (Ground.GetMaximumX() >> Coordinate::kBitShiftLocalX) + mReach.GetX();
            const SInt32 MaxRegionY = (Ground.GetMaximumY() >> Coordinate::kBitShiftLocalY) + mReach.GetY();

            // Probing every region a wide query spans costs more than sweeping the few that are resident.
            const UInt64 Span = static_cast<UInt64>(MaxRegionX - MinRegionX + 1)
                              * static_cast<UInt64>(MaxRegionY - MinRegionY + 1);

            if (Span > mRegistry.GetSize())
            {
                for (ConstRef<decltype(mRegistry)::Pair> Pair : mRegistry)
                {
                    Ref<Slot> Slot = (* Pair.Second);

                    if (IntBox::Overlaps(Volume, Slot.Boundaries) && Predicate(Slot))
                    {
                        return true;
                    }
                }
                return false;
            }

            for (SInt32 RegionY = MinRegionY; RegionY <= MaxRegionY; ++RegionY)
            {
                for (SInt32 RegionX = MinRegionX; RegionX <= MaxRegionX; ++RegionX)
                {
                    const Ptr<Slot> Slot = FindSlot(RegionX, RegionY);

                    // Early reject the whole slot on the union of the cells it holds.
                    if (!Slot || !IntBox::Overlaps(Volume, Slot->Boundaries))
                    {
                        continue;
                    }

                    if (Predicate(* Slot))
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        /// \brief Files an entity under the cell its volume falls in, and records which one on the enclosure.
        ///
        /// \param Actor     The entity to file.
        /// \param Enclosure The entity's enclosure, which is given the cell it was filed under.
        /// \param Center    The center coordinates of the entity's volume.
        void InsertEntityOnCell(Scene::Entity Actor, Ref<Enclosure> Enclosure, IntVector2 Center);

        /// \brief Unfiles an entity from the cell its enclosure was filed under.
        ///
        /// \param Actor     The entity to unfile.
        /// \param Enclosure The entity's enclosure, which is left unlinked.
        void RemoveEntityOnCell(Scene::Entity Actor, Ref<Enclosure> Enclosure);

        /// \brief Moves an entity to the cell its new volume falls in, if that is a different one.
        ///
        /// \param Actor        The entity to move.
        /// \param Enclosure    The entity's enclosure, which is given the cell it ends up under.
        /// \param NewestCenter The new center coordinates of the entity's volume after the update.
        void UpdateEntityOnCell(Scene::Entity Actor, Ref<Enclosure> Enclosure, IntVector2 NewestCenter);

        /// \brief Recursively unlinks an entity and all of its descendants from their spatial cells.
        ///
        /// \param Root The subtree root to detach; call this before destroying the subtree.
        void DetachEntityOnCell(Scene::Entity Root);

        /// \brief Retires a slot, saving and destroying its region and clearing every reference to its cells.
        ///
        /// \param Slot The slot to retire, which is left ready to be dropped from the registry.
        void Evict(Ref<Slot> Slot);

        /// \brief Handles asynchronous load result of a \ref Region.
        ///
        /// \param Result          The result of the asynchronous load operation.
        /// \param File            The loaded file blob.
        /// \param Handle          The handle of the entity associated with the load operation.
        /// \param RegionX         The X coordinate of the region.
        /// \param RegionY         The Y coordinate of the region.
        /// \param CreateIfMissing Whether to create the region if it is missing.
        void OnAsyncLoad(Filesystem::Result Result, Blob File, UInt64 Handle, SInt16 RegionX, SInt16 RegionY, Bool CreateIfMissing);

    private:

        /// \brief Packs a pair of signed cell coordinates into the key they are stored under.
        ///
        /// \param X The X-coordinate of the cell.
        /// \param Y The Y-coordinate of the cell.
        /// \return The unique key representing the cell.
        ZY_INLINE static constexpr UInt32 GetKey(SInt32 X, SInt32 Y)
        {
            return (static_cast<UInt32>(static_cast<UInt16>(X)) << 16u) | static_cast<UInt16>(Y);
        }

        /// \brief Unpacks a key back into the cell coordinates it was built from.
        ///
        /// \param Key The key to unpack.
        /// \return The coordinates the key represents.
        ZY_INLINE static constexpr IntVector2 GetKeyCoordinate(UInt32 Key)
        {
            return IntVector2(static_cast<SInt16>(Key >> 16u), static_cast<SInt16>(Key & 0xFFFFu));
        }

        /// \brief Packs the loose cell a coordinate falls in into its slot and its index within that slot.
        ///
        /// \param Loose The loose cell coordinates, in loose cell space.
        /// \return The key identifying that cell across the whole world.
        ZY_INLINE static constexpr UInt64 GetLooseKey(IntVector2 Loose)
        {
            const UInt32 Region = GetKey(Loose.GetX() >> kLooseShiftX, Loose.GetY() >> kLooseShiftY);
            const UInt32 Local  = ConvertTo1D<UInt32>(
                Loose.GetX() & (kLooseSizeX - 1),
                Loose.GetY() & (kLooseSizeY - 1),
                kLooseSizeX);
            return (static_cast<UInt64>(Region) << 32u) | Local;
        }

        /// \brief Gets the loose cell a world position falls in, in loose cell coordinates.
        ///
        /// \param Center The center coordinates of the entity's volume.
        /// \return The coordinates of the cell containing that position.
        ZY_INLINE static constexpr IntVector2 GetLooseCoordinate(IntVector2 Center)
        {
            return Center >> kHierarchyLooseLog;
        }

        /// \brief Gets how far a slot's boundaries reach past the region it holds, in regions.
        ///
        /// \param Origin     The coordinates of the region the slot holds.
        /// \param Boundaries The union of the slot's cell boundaries.
        /// \return The reach on each axis, which is zero when nothing spills out.
        ZY_INLINE static constexpr IntVector2 GetReach(IntVector2 Origin, ConstRef<IntBox> Boundaries)
        {
            if (Boundaries != IntBox::Zero())
            {
                const SInt32 MinX   = Origin.GetX() << Coordinate::kBitShiftLocalX;
                const SInt32 MinY   = Origin.GetY() << Coordinate::kBitShiftLocalY;

                // How far the boundaries pass each edge of the region, staying negative while they sit inside it.
                const SInt32 LowerX = MinX - Boundaries.GetMinimumX();
                const SInt32 UpperX = Boundaries.GetMaximumX() - (MinX + (1 << Coordinate::kBitShiftLocalX));
                const SInt32 LowerY = MinY - Boundaries.GetMinimumZ();
                const SInt32 UpperY = Boundaries.GetMaximumZ() - (MinY + (1 << Coordinate::kBitShiftLocalY));

                return IntVector2(
                    (Max(0, LowerX, UpperX) + (1 << Coordinate::kBitShiftLocalX) - 1) >> Coordinate::kBitShiftLocalX,
                    (Max(0, LowerY, UpperY) + (1 << Coordinate::kBitShiftLocalY) - 1) >> Coordinate::kBitShiftLocalY);
            }
            return IntVector2::Zero();
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Table<UInt32, Unique<Slot>> mRegistry;
        IntVector2                  mReach;
        Bag<UInt32>                 mManifest;
    };
}