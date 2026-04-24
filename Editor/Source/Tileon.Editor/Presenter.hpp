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

#include <imgui.h>
#include <imgui_internal.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Provides utility functions for rendering user interface elements in the editor.
    class Presenter final
    {
    public:

        /// \brief Initializes the presenter, setting up the theme and style for the user interface.
        Presenter();

        /// \brief Renders a label, aligning the text to the frame padding for consistent spacing.
        template<typename... Arguments>
        ZYPHRYON_INLINE void Label(ConstStr8 Format, AnyRef<Arguments>... Parameters)
        {
            ImGui::AlignTextToFramePadding();

            const ConstStr8 Text = Base::Format(Format, Parameters...);
            ImGui::TextUnformatted(Text.data(), Text.data() + Text.size());
        }

        /// \brief Renders a button with the specified label and dimensions, and invokes the provided callback when clicked.
        template<typename Callback>
        ZYPHRYON_INLINE Bool Button(ConstStr8 Label, AnyRef<Callback> OnAction, Real32 Width, Real32 Height)
        {
            const Bool Clicked = ImGui::Button(Label.data(), ImVec2(Width, Height));

            if (Clicked && OnAction)
            {
                OnAction();
            }
            return Clicked;
        }

        /// \brief Renders a checkbox with the specified label and state, allowing the user to toggle the state on or off.
        ZYPHRYON_INLINE Bool Checkbox(ConstStr8 Label, Ref<Bool> State)
        {
            return ImGui::Checkbox(Label.data(), &State);
        }
    };
}