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

#include "Tileon.Runtime/Controller.hpp"
#include "Component/Registry.hpp"
#include "Project.hpp"
#include "Session.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Represents the context for the editor, providing access to various services.
    class Context final : public Session, public Engine::Locator<Scene::Service, Content::Service, Graphic::Service>
    {
    public:

        /// \brief Constructs a context instance with the specified service host.
        ///
        /// \param Host    The service host to associate with the context.
        /// \param Project The project to load into the context.
        explicit Context(Ref<Engine::Subsystem::Host> Host, AnyRef<Project> Project);

        /// \brief Tears down the context, releasing any resources it holds.
        void Teardown();

        /// \brief Gets a reference to the scene service associated with the context.
        ///
        /// \return A reference to the scene service associated with the context.
        ZY_INLINE Ref<Scene::Service> GetScene()
        {
            return GetService<Scene::Service>();
        }

        /// \brief Gets a reference to the content service associated with the context.
        ///
        /// \return A reference to the content service associated with the context.
        ZY_INLINE Ref<Content::Service> GetContent()
        {
            return GetService<Content::Service>();
        }

        /// \brief Gets a reference to the graphic service associated with the context.
        ///
        /// \return A reference to the graphic service associated with the context.
        ZY_INLINE Ref<Graphic::Service> GetGraphic()
        {
            return GetService<Graphic::Service>();
        }

        /// \brief Gets a reference to the controller associated with the context.
        ///
        /// \return A reference to the controller associated with the context.
        ZY_INLINE Ref<Controller> GetController()
        {
            return mController;
        }

        /// \brief Gets a reference to the repository from the world associated with the controller.
        ///
        /// \return A reference to the repository from the world associated with the controller.
        ZY_INLINE Ref<Repository> GetRepository()
        {
            return mController.GetWorld().GetRepository();
        }

        /// \brief Gets a reference to the supervisor from the world associated with the controller.
        ///
        /// \return A reference to the supervisor from the world associated with the controller.
        ZY_INLINE Ref<Supervisor> GetSupervisor()
        {
            return mController.GetWorld().GetSupervisor();
        }

        /// \brief Gets a reference to the director from the controller.
        ///
        /// \return A reference to the director from the controller.
        ZY_INLINE Ref<Director> GetDirector()
        {
            return mController.GetDirector();
        }

        /// \brief Gets a reference to the renderer from the controller.
        ///
        /// \return A reference to the renderer from the controller.
        ZY_INLINE Ref<Renderer> GetRenderer()
        {
            return mController.GetRenderer();
        }

        /// \brief Gets a reference to the tileset from the renderer associated with the controller.
        ///
        /// \return A reference to the tileset from the renderer associated with the controller.
        ZY_INLINE Ref<Tileset> GetTileset()
        {
            return mController.GetRenderer().GetTileset();
        }

        /// \brief Gets a reference to the registry of inspectable components.
        ///
        /// \return A reference to the registry associated with the context.
        ZY_INLINE Ref<Registry> GetRegistry()
        {
            return mRegistry;
        }

    private:

        /// \brief Restores the persisted editor session for the current project, including the camera position.
        void Load();

        /// \brief Persists the editor session for the current project, including the camera position.
        void Save();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Controller mController;
        Project    mProject;
        Registry   mRegistry;
    };
}