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
#include <Zyphryon.Graphic/Common.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::UI
{
    /// \brief Provides utility functions for rendering user interface elements in the editor.
    class Composer final
    {
    public:

        /// \brief Begins a new main menu bar, allowing the user to create a menu structure for the editor.
        ZYPHRYON_INLINE Bool BeginMainMenuBar()
        {
            return ImGui::BeginMainMenuBar();
        }

        /// \brief Ends the current main menu bar, finalizing the menu structure for the editor.
        ZYPHRYON_INLINE void EndMainMenuBar()
        {
            ImGui::EndMainMenuBar();
        }

        /// \brief Begins a new menu with the specified label and enabled state.
        ZYPHRYON_INLINE Bool BeginMenu(ConstStr8 Label, Bool Enabled = true)
        {
            return ImGui::BeginMenu(Label.data(), Enabled);
        }

        /// \brief Ends the current menu, finalizing the submenu structure.
        ZYPHRYON_INLINE void EndMenu()
        {
            ImGui::EndMenu();
        }

        /// \brief Renders a menu item with the specified label, shortcut, and enabled state, and returns whether it was selected.
        ZYPHRYON_INLINE Bool MenuItem(ConstStr8 Label, ConstStr8 Shortcut = {}, Bool Enabled = true)
        {
            return ImGui::MenuItem(Label.data(), Shortcut.empty() ? nullptr : Shortcut.data(), false, Enabled);
        }

        /// \brief Renders a separator line to visually separate groups of menu items or interface elements.
        ZYPHRYON_INLINE void Separator()
        {
            ImGui::Separator();
        }

        /// \brief Renders a label, aligning the text to the frame padding for consistent spacing.
        template<typename... Arguments>
        ZYPHRYON_INLINE void Label(ConstStr8 Format, AnyRef<Arguments>... Parameters)
        {
            ImGui::AlignTextToFramePadding();

            const ConstStr8 Text = Base::Format(Format, Parameters...);
            ImGui::TextUnformatted(Text.data(), Text.data() + Text.size());
        }

        /// \brief Renders a button with the specified label and size, returning whether it was clicked by the user.
        ZYPHRYON_INLINE Bool Button(ConstStr8 Label, Real32 Width = 0.0f, Real32 Height = 0.0f)
        {
            return ImGui::Button(Label.data(), ImVec2(Width, Height));
        }

        /// \brief Renders a checkbox with the specified label and state, allowing the user to toggle the state on or off.
        ZYPHRYON_INLINE Bool Checkbox(ConstStr8 Label, Ref<Bool> State)
        {
            return ImGui::Checkbox(Label.data(), &State);
        }

        /// \brief Renders a window with the specified title, and flags, returning whether the window is currently open.
        ZYPHRYON_INLINE Bool Begin(ConstStr8 Title, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            return ImGui::Begin(Title.data(), nullptr, Flags);
        }

        /// \brief Renders a window with the specified title, open state, and flags, returning whether the window is currently open.
        ZYPHRYON_INLINE Bool Begin(ConstStr8 Title, Ref<Bool> Open, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            return ImGui::Begin(Title.data(), &Open, Flags);
        }

        /// \brief Ends the current window, finalizing its layout and rendering it on the screen.
        ZYPHRYON_INLINE void End()
        {
            ImGui::End();
        }

        /// \brief Begins a child window with the specified ID, size, flags, and window flags, allowing for nested interface elements within a parent window.
        ZYPHRYON_INLINE void BeginChild(ConstStr8 ID, ImVec2 Size = ImVec2(0, 0), ImGuiChildFlags ChildFlags = ImGuiChildFlags_None, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            ImGui::BeginChild(ID.data(), Size, ChildFlags, Flags);
        }

        /// \brief Ends the current child window, finalizing its layout and rendering it within the parent window.
        ZYPHRYON_INLINE void EndChild()
        {
            ImGui::EndChild();
        }

        /// \brief Continue rendering on the same line as the previous item, allowing for horizontal layout of interface elements.
        ZYPHRYON_INLINE void SameLine()
        {
            ImGui::SameLine();
        }

        /// \brief Sets the size of the next window to be rendered, with an optional condition for when the size should be applied.
        ZYPHRYON_INLINE void SetNextWindowSize(Real32 Width, Real32 Height, ImGuiCond Condition = ImGuiCond_None)
        {
            ImGui::SetNextWindowSize(ImVec2(Width, Height), Condition);
        }

        /// \brief Sets the size constraints for the next window to be rendered, specifying minimum and maximum dimensions.
        ZYPHRYON_INLINE void SetNextWindowSizeConstraints(Real32 MinWidth, Real32 MinHeight, Real32 MaxWidth = FLT_MAX, Real32 MaxHeight = FLT_MAX)
        {
            ImGui::SetNextWindowSizeConstraints(ImVec2(MinWidth, MinHeight), ImVec2(MaxWidth, MaxHeight));
        }

        /// \brief Renders an image using the specified texture ID, dimensions, and optional UV coordinates for texture mapping.
        ZYPHRYON_INLINE void Image(Graphic::Object ID, Real32 Width, Real32 Height, ImVec4 UV = ImVec4(0, 0, 1, 1))
        {
            const ImVec2 UV0(UV.x, UV.y);
            const ImVec2 UV1(UV.z, UV.w);
            ImGui::Image(ID, ImVec2(Width, Height), UV0, UV1);
        }

        /// \brief Gets the available size of the content region within the current window.
        ZYPHRYON_INLINE ImVec2 GetContentRegionAvail() const
        {
            return ImGui::GetContentRegionAvail();
        }
    };
}