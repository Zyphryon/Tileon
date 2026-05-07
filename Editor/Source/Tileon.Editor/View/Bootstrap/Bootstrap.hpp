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

#include "Tileon.Editor/Project.hpp"
#include "Tileon.Editor.UI/Composer.hpp"
#include <Zyphryon.Engine/Device.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    /// \brief Represents the bootstrap activity, provides an entry point for launching projects.
    class Bootstrap final
    {
    public:

        /// \brief The result of the bootstrap activity, indicating the next action to take after drawing the interface.
        enum class Result : UInt8
        {
            None,   ///< No action, continue drawing the bootstrap interface.
            Done,   ///< A project was launched, exit the bootstrap and transition to the editor view.
            Exit,   ///< The user chose to exit the application, signal to terminate the editor.
        };

    public:

        /// \brief Constructs the bootstrap activity, initializing its state to the main menu.
        Bootstrap();

        /// \brief Gets the project associated with the bootstrap activity.
        ///
        /// \return The project associated with the bootstrap activity.
        ZYPHRYON_INLINE AnyRef<Project> GetProject()
        {
            return Move(mProject);
        }

        /// \brief Draws the bootstrap activity, rendering the user interface for launching projects.
        ///
        /// \param Composer The UI composer used to render the bootstrap interface elements.
        /// \param Device   The window device used for rendering the bootstrap interface.
        /// \return The result of the operation, indicating whether to continue, exit, or launch a project.
        Result Draw(Ref<UI::Composer> Composer, ConstRef<Engine::Device> Device);

    private:

        /// \brief Represents the current state of the bootstrap activity, determining which interface to display.
        enum class State : UInt8
        {
            Menu,   ///< The main menu state, where the user can choose to create a new project or open an existing one.
            Wizard, ///< The project creation wizard state, where the user can configure settings for a new project.
            Done,   ///< The done state, indicating that a project has been launched.
        };

        /// \brief Draws the main menu interface of the bootstrap activity.
        ///
        /// \param Composer The UI composer used to render the menu interface elements.
        /// \param Device   The window device used for rendering the menu interface.
        /// \return The result of the operation, indicating whether to continue, exit, or launch a project.
        Result DrawInMenu(Ref<UI::Composer> Composer, ConstRef<Engine::Device> Device);

        /// \brief Draws the project creation wizard interface of the bootstrap activity.
        ///
        /// \param Composer The UI composer used to render the wizard interface elements.
        /// \param Device   The window device used for rendering the wizard interface.
        /// \return The result of the operation, indicating whether to continue, exit, or launch a project.
        Result DrawInWizard(Ref<UI::Composer> Composer, ConstRef<Engine::Device> Device);

        /// \brief Opens a file dialog to allow the user to select an existing project file to open.
        ///
        /// \param Device The window device used for rendering the file dialog interface.
        void OpenFileDialog(ConstRef<Engine::Device> Device);

        /// \brief Opens a file dialog to allow the user to specify a location and name for saving a new project.
        ///
        /// \param Device The window device used for rendering the file dialog interface.
        void OpenSaveDialog(ConstRef<Engine::Device> Device);

        /// \brief Handles the result of a file dialog operation.
        ///
        /// \param Path The file path returned from the file dialog.
        void OnDialogResult(ConstStr8 Path);

        /// \brief Prepare the project directory by copying necessary bootstrap files.
        ///
        /// \param Path The file path of the project to prepare.
        /// \return `true` if the project directory was successfully prepared, otherwise `false`.
        Bool Scaffold(ConstStr8 Path);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        State   mState;
        Project mProject;
    };
}