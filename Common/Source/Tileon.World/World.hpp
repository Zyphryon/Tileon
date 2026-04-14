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

#include "Repository.hpp"
#include "Supervisor.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents the main world class, managing the game state, entities, and services.
    class World : public Locator<Scene::Service>
    {
    public:

        /// \brief Constructs a World instance with the specified service host.
        ///
        /// \param Host The service host to associate with the World.
        explicit World(Ref<Service::Host> Host);

        /// \brief Tears down the World, releasing resources and shutting down services.
        void Teardown();

        /// \brief Loads the World state, initializing entities and resources.
        void Load();

        /// \brief Saves the current state of the World.
        void Save();

        /// \brief Retrieves the Repository instance associated with the World.
        ///
        /// \return A reference to the Repository.
        ZYPHRYON_INLINE Ref<Repository> GetRepository()
        {
            return mRepository;
        }

        /// \brief Retrieves the Supervisor instance associated with the World.
        ///
        /// \return A reference to the Supervisor.
        ZYPHRYON_INLINE Ref<Supervisor> GetSupervisor()
        {
            return mSupervisor;
        }

    private:

        /// \brief Registers the World with the specified Scene service.
        ///
        /// \param Scene The Scene service to register with.
        void OnRegister(Ref<Scene::Service> Scene);

        /// \brief Tears down the World from the specified Scene service.
        ///
        /// \param Scene The Scene service to tear down from.
        void OnTeardown(Ref<Scene::Service> Scene);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Repository mRepository;
        Supervisor mSupervisor;
    };
}
