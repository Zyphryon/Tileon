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

#include <Zyphryon.Content/Service.hpp>
#include <Zyphryon.Scene/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Manages the repository of terrains and archetypes in the world.
    class Repository final : public Engine::Locator<Job::Service, Scene::Service, Content::Service>
    {
    public:

        /// \brief Default filename for storing the world state.
        static constexpr Text   kManifestUri    = "Resources://Data/World.bin";

        /// \brief Maximum number of entity archetypes that can be stored in the repository.
        static constexpr UInt32 kArchetypeLimit = Scene::kMaxCountArchetypes;

        /// \brief Default filename for storing entity archetypes.
        static constexpr Text   kArchetypeUri   = "Resources://Data/Archetypes.bin";

    public:

        /// \brief Constructs a repository with the specified service host.
        ///
        /// \param Host The service host to associate with the repository.
        explicit Repository(Ref<Engine::Subsystem::Host> Host);

        /// \brief Loads the repository data.
        void Load();

        /// \brief Saves the repository data.
        void Save();

        /// \brief Creates a new entity archetype in the repository.
        ///
        /// \return The archetype that was created.
        Scene::Archetype CreateArchetype();

        /// \brief Deletes an entity archetype from the repository.
        ///
        /// \param Archetype The archetype to delete.
        void DeleteArchetype(Scene::Archetype Archetype);

        /// \brief Attaches an archetype as a fixed part of another and propagates it to live instances.
        ///
        /// \param Parent The archetype to receive the part.
        /// \param Part   The archetype to attach.
        void AttachArchetype(Scene::Archetype Parent, Scene::Archetype Part);

        /// \brief Propagates a component change made on an archetype to every instance already spawned from it.
        ///
        /// \param Archetype The archetype whose component set changed.
        void RefreshArchetype(Scene::Archetype Archetype);

        /// \brief Gets an entity archetype by its unique identifier.
        ///
        /// \param ID The unique identifier of the archetype.
        /// \return The entity archetype corresponding to the given ID.
        ZY_INLINE Scene::Entity GetArchetype(UInt64 ID) const
        {
            return GetService<Scene::Service>().GetEntity(ID);
        }

        /// \brief Gets an entity archetype by its unique name.
        ///
        /// \param Name The name of the archetype.
        /// \return The entity archetype corresponding to the given name.
        ZY_INLINE Scene::Entity GetArchetype(Text Name) const
        {
            return GetService<Scene::Service>().GetEntity(Name);
        }

        /// \brief Iterates over all entity archetypes.
        ///
        /// \param Callback The callback function to apply to each archetype entity.
        template<typename Function>
        ZY_INLINE void ForEachArchetype(AnyRef<Function> Callback) const
        {
            GetService<Scene::Service>().QueryArchetypes(Callback);
        }

    private:

        /// \brief Loads the world manifest from file.
        void LoadManifest(Filesystem::Result Result, AnyRef<Blob> Data);

        /// \brief Saves the world manifest to file.
        void SaveManifest();

        /// \brief Loads the entity archetype database from file.
        void LoadArchetypeDatabase(Filesystem::Result Result, AnyRef<Blob> Data);

        /// \brief Saves the entity archetype database to file.
        void SaveArchetypeDatabase();
    };
}