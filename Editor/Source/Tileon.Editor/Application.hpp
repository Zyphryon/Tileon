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

#include "Activity.hpp"
#include "View/Bootstrap/Bootstrap.hpp"
#include <ImGuiSystem.hpp>
#include <Zyphryon.Engine/Kernel.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Represents the main application kernel for the Tileon editor.
    class Application final : public Engine::Kernel
    {
    public:

        /// \brief
        Application();

        /// \copydoc Kernel::OnInitialize
        Bool OnInitialize() override;

        /// \copydoc Kernel::OnTeardown
        void OnTeardown() override;

        /// \copydoc Kernel::OnTick
        void OnTick(Time Time) override;

    private:

        /// \brief Enumerates the different states of the editor application.
        enum class State : UInt8
        {
            Idle,       ///< The application is idle, waiting for user input or project loading.
            Loading,    ///< The application is loading assets and initializing the editor state for the current project.
            Running,    ///< The application is running, actively processing user input and updating the editor state.
        };

        /// \brief Launches the specified project, initializing the editor state and loading the project data.
        ///
        /// \param Project The project to launch in the editor.
        void Launch(AnyRef<Project> Project);

        /// \brief Draws the editor view, which contains the main user interface for editing and manipulating the game world.
        ///
        /// \param Composer The UI composer used to render the editor interface elements.
        /// \param Time     The time elapsed since the last tick.
        void DrawEditor(Ref<UI::Composer> Composer, Time Time);

        /// \brief Draws the game view, rendering the current state of the game world.
        void DrawGame();

        /// \brief Draws a simple loading overlay while assets are being streamed in.
        ///
        /// \param Composer The UI composer used to render the loading screen.
        void DrawLoading(Ref<UI::Composer> Composer);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        State                     mState;
        View::Bootstrap           mBootstrap;
        Unique<Context>           mContext;
        Plugin::ImGuiSystem       mFrontend;
        Vector<Tracker<Activity>> mActivities;
        IntVector2                mViewport;
    };
}