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

        /// \copydoc Kernel::OnInitialize
        Bool OnInitialize() override;

        /// \copydoc Kernel::OnTeardown
        void OnTeardown() override;

        /// \copydoc Kernel::OnTick
        void OnTick(Time Time) override;

    private:

        /// \brief Initializes the user interface theme, setting up colors and styles for the editor's appearance.
        void InitializeTheme();

        /// \brief Sets the dark theme for the user interface.
        void SetDarkTheme();

        /// \brief Sets the light theme for the user interface.
        void SetLightTheme();

        /// \brief Draws the user interface, rendering any relevant information and controls for the user.
        ///
        /// \param Time The time elapsed since the last tick.
        void DrawInterface(Time Time);

        /// \brief Draws the game view, rendering the current state of the game world.
        ///
        /// \param Width  The width of the game view in pixels.
        /// \param Height The height of the game view in pixels.
        void DrawGame(UInt16 Width, UInt16 Height);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Unique<Context>           mContext;
        Plugin::ImGuiSystem       mFrontend;
        Vector<Tracker<Activity>> mActivities;
        IntVector2                mViewport;
    };
}