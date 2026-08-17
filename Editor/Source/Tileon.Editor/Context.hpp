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

#include "Tileon.Session/Presenter.hpp"
#include "Inspect/ComponentCatalog.hpp"
#include "History.hpp"
#include "Project.hpp"
#include "Session.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Represents the context for the editor, providing access to various services.
    class Context final : public Session, Engine::Locator<Content::Service, Graphic::Service, Job::Service,  Scene::Service>
    {
    public:

        /// \brief Constructs a context instance with the specified service host.
        ///
        /// \param Host    The service host to associate with the context.
        /// \param Project The project to load into the context.
        explicit Context(Ref<Engine::Subsystem::Host> Host, AnyRef<Project> Project);

        /// \brief Tears down the context, releasing any resources it holds.
        void Teardown();

        /// \brief Writes the project to disk, baking the tileset's arrays before its database names them.
        void Commit();

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

        /// \brief Gets a reference to the scheduler the in-process bakers run their work on.
        ///
        /// \return A reference to the job service associated with the context.
        ZY_INLINE Ref<Job::Service> GetScheduler()
        {
            return GetService<Job::Service>();
        }

        /// \brief Gets a reference to the scene service associated with the context.
        ///
        /// \return A reference to the scene service associated with the context.
        ZY_INLINE Ref<Scene::Service> GetScene()
        {
            return GetService<Scene::Service>();
        }

        /// \brief Gets a reference to the controller associated with the context.
        ///
        /// \return A reference to the controller associated with the context.
        ZY_INLINE Ref<Presenter> GetPresenter()
        {
            return mPresenter;
        }

        /// \brief Gets a reference to the repository from the world associated with the controller.
        ///
        /// \return A reference to the repository from the world associated with the controller.
        ZY_INLINE Ref<Repository> GetRepository()
        {
            return mPresenter.GetWorld().GetRepository();
        }

        /// \brief Gets a reference to the supervisor from the world associated with the controller.
        ///
        /// \return A reference to the supervisor from the world associated with the controller.
        ZY_INLINE Ref<Supervisor> GetSupervisor()
        {
            return mPresenter.GetWorld().GetSupervisor();
        }

        /// \brief Gets a reference to the director from the controller.
        ///
        /// \return A reference to the director from the controller.
        ZY_INLINE Ref<Director> GetDirector()
        {
            return mPresenter.GetDirector();
        }

        /// \brief Gets a reference to the renderer from the controller.
        ///
        /// \return A reference to the renderer from the controller.
        ZY_INLINE Ref<Renderer> GetRenderer()
        {
            return mPresenter.GetRenderer();
        }

        /// \brief Gets a reference to the registry of inspectable components.
        ///
        /// \return A reference to the registry associated with the context.
        ZY_INLINE Ref<ComponentCatalog> GetCatalog()
        {
            return mCatalog;
        }

        /// \brief Gets a reference to the project the editor has open.
        ///
        /// \return A reference to the project associated with the context.
        ZY_INLINE Ref<Project> GetProject()
        {
            return mProject;
        }

        /// \brief Gets a reference to the undo stack every edit is recorded into.
        ///
        /// \return A reference to the history associated with the context.
        ZY_INLINE Ref<History> GetHistory()
        {
            return mHistory;
        }

    private:

        /// \brief Restores the persisted editor session for the current project, including the camera position.
        void Load();

        /// \brief Persists the editor session for the current project, including the camera position.
        void Save();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Presenter        mPresenter;
        Project          mProject;
        ComponentCatalog mCatalog;
        History          mHistory;
    };
}