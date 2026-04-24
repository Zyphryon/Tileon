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

#include "Tileon.Render/Controller.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Represents the context for the editor, providing access to various services.
    class Context final : public Locator<Scene::Service, Content::Service>
    {
    public:

        /// \brief Constructs a context instance with the specified service host.
        ///
        /// \param Host The service host to associate with the context.
        explicit Context(Ref<Service::Host> Host);

        /// \brief Tears down the context, releasing any resources it holds.
        void Teardown();

        /// \brief Polls the context for updates, allowing it to process any pending operations or events.
        void Poll();

        /// \brief Gets a reference to the controller associated with the context.
        ///
        /// \return A reference to the controller associated with the context.
        ZYPHRYON_INLINE Ref<Controller> GetController()
        {
            return mController;
        }

        /// \brief Gets a reference to the repository from the world associated with the controller.
        ///
        /// \return A reference to the repository from the world associated with the controller.
        ZYPHRYON_INLINE Ref<Repository> GetRepository()
        {
            return mController.GetWorld().GetRepository();
        }

        /// \brief Gets a reference to the supervisor from the world associated with the controller.
        ///
        /// \return A reference to the supervisor from the world associated with the controller.
        ZYPHRYON_INLINE Ref<Supervisor> GetSupervisor()
        {
            return mController.GetWorld().GetSupervisor();
        }

        /// \brief Gets a reference to the director from the controller.
        ///
        /// \return A reference to the director from the controller.
        ZYPHRYON_INLINE Ref<Director> GetDirector()
        {
            return mController.GetDirector();
        }

        /// \brief Gets a reference to the renderer from the controller.
        ///
        /// \return A reference to the renderer from the controller.
        ZYPHRYON_INLINE Ref<Renderer> GetRenderer()
        {
            return mController.GetRenderer();
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Controller mController;
    };
}