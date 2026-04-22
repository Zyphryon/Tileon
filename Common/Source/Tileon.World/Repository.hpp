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

#include "Terrain.hpp"
#include <Zyphryon.Content/Service.hpp>
#include <Zyphryon.Scene/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Manages the repository of terrains and archetypes in the world.
    class Repository final : public Locator<Scene::Service, Content::Service>
    {
    public:

        /// \brief Maximum number of terrains that can be stored in the repository.
        static constexpr UInt32    kTerrainLimit      = 4'096;

        /// \brief Default filename for storing terrain data.
        static constexpr ConstStr8 kTerrainFilename   = "Resources://Data/Terrains.bin";

        /// \brief Maximum number of archetypes that can be stored in the repository.
        static constexpr UInt32    kArchetypeLimit    = Scene::kMaxCountArchetypes;

        /// \brief Default filename for storing archetype data.
        static constexpr ConstStr8 kArchetypeFilename = "Resources://Data/Archetypes.bin";

    public:

        /// \brief Constructs a repository with the specified service host.
        ///
        /// \param Host The service host to associate with the repository.
        explicit Repository(Ref<Service::Host> Host);

        /// \brief Loads the terrain and archetype data from their respective files.
        void Load();

        /// \brief Saves the terrain and archetype data to their respective files.
        void Save();

        /// \brief Creates a new archetype in the repository.
        ///
        /// \return The created archetype entity.
        Scene::Entity CreateArchetype();

        /// \brief Deletes an archetype from the repository.
        ///
        /// \param Archetype The archetype entity to delete.
        void DeleteArchetype(Scene::Entity Archetype);

        /// \brief Gets an archetype by its unique identifier.
        ///
        /// \param ID The unique identifier of the archetype.
        /// \return The archetype entity corresponding to the given ID.
        ZYPHRYON_INLINE Scene::Entity GetArchetype(UInt64 ID) const
        {
            return GetService<Scene::Service>().GetEntity(ID);
        }

        /// \brief Gets an archetype by its name.
        ///
        /// \param Name The name of the archetype.
        /// \return The archetype entity corresponding to the given name.
        ZYPHRYON_INLINE Scene::Entity GetArchetype(ConstStr8 Name) const
        {
            return GetService<Scene::Service>().GetEntity(Name);
        }

        /// \brief Creates a new terrain in the repository.
        ///
        /// \return A reference to the created terrain.
        Ref<Terrain> CreateTerrain();

        /// \brief Deletes a terrain from the repository.
        ///
        /// \param Terrain The terrain to delete.
        void DeleteTerrain(Ref<Terrain> Terrain);

        /// \brief Gets a terrain by its unique identifier.
        ///
        /// \param ID The unique identifier of the terrain.
        /// \return A reference to the terrain corresponding to the given ID.
        ZYPHRYON_INLINE Ref<Terrain> GetTerrain(UInt16 ID)
        {
            return mTerrains[ID];
        }

        /// \brief Gets a terrain by its unique identifier.
        ///
        /// \param ID The unique identifier of the terrain.
        /// \return A constant reference to the terrain corresponding to the given ID.
        ZYPHRYON_INLINE ConstRef<Terrain> GetTerrain(UInt16 ID) const
        {
            return mTerrains[ID];
        }

        /// \brief Iterates over all archetypes and applies the given callback function.
        ///
        /// \param Callback The callback function to apply to each archetype.
        template<typename Function>
        ZYPHRYON_INLINE void ForEachArchetype(AnyRef<Function> Callback) const
        {
            GetService<Scene::Service>().QueryArchetypes(Callback);
        }

        /// \brief Iterates over all terrains and applies the given callback function.
        ///
        /// \param Callback The callback function to apply to each terrain.
        template<typename Function>
        ZYPHRYON_INLINE void ForEachTerrain(AnyRef<Function> Callback) const
        {
            mTerrains.ForEach(Callback);
        }

    private:

        /// \brief Loads the archetype database from file.
        ///
        /// \return `true` if the database was loaded successfully, `false` otherwise.
        Bool LoadArchetypeDatabase();

        /// \brief Saves the archetype database to file.
        void SaveArchetypeDatabase();

        /// \brief Loads the terrain database from file.
        ///
        /// \return `true` if the database was loaded successfully, `false` otherwise.
        Bool LoadTerrainDatabase();

        /// \brief Saves the terrain database to file.
        void SaveTerrainDatabase();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Pool<Terrain, kTerrainLimit> mTerrains;
    };
}
