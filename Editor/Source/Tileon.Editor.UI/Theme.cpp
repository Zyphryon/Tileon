// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Theme.hpp"
#include "Icon.hpp"
#include <imgui.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Theme::Initialize()
    {
        // Configure font settings for the user interface.
        ImFontConfig Config;
        Config.MergeMode        = true;
        Config.PixelSnapH       = true;
        Config.GlyphMinAdvanceX = 13.0f;

        constexpr ImWchar kIconRange[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

        const Ptr<ImFontAtlas> Atlas = ImGui::GetIO().Fonts;
        Atlas->Clear();
        Atlas->AddFontFromFileTTF("Editor\\Fonts\\Roboto-Medium.ttf",  16.0f);
        Atlas->AddFontFromFileTTF("Editor\\Fonts\\fa-regular-400.ttf", 16.0f, &Config, kIconRange);
        Atlas->AddFontFromFileTTF("Editor\\Fonts\\fa-solid-900.ttf",   16.0f, &Config, kIconRange);
        Atlas->Build();

        // Allow moving windows by dragging only the title bar.
        ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

        // Apply the dark theme by default.
        ApplyDarkStyle();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Theme::ApplyDarkStyle()
    {
        ImGui::StyleColorsDark();

        Ref<ImGuiStyle> Style = ImGui::GetStyle();
        Style.WindowRounding    = 8.0f;
        Style.ChildRounding     = 8.0f;
        Style.FrameRounding     = 6.0f;
        Style.PopupRounding     = 6.0f;
        Style.ScrollbarRounding = 6.0f;
        Style.GrabRounding      = 6.0f;
        Style.TabRounding       = 6.0f;

        const Ptr<ImVec4> Colors = Style.Colors;
        Colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        Colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        Colors[ImGuiCol_WindowBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        Colors[ImGuiCol_ChildBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        Colors[ImGuiCol_PopupBg]               = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        Colors[ImGuiCol_Border]                = ImVec4(0.25f, 0.25f, 0.25f, 0.70f);
        Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        Colors[ImGuiCol_FrameBg]               = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        Colors[ImGuiCol_TitleBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
        Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        Colors[ImGuiCol_CheckMark]             = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        Colors[ImGuiCol_SliderGrab]            = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
        Colors[ImGuiCol_Button]                = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        Colors[ImGuiCol_ButtonActive]          = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        Colors[ImGuiCol_Header]                = ImVec4(0.25f, 0.25f, 0.25f, 0.55f);
        Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.35f, 0.35f, 0.35f, 0.80f);
        Colors[ImGuiCol_HeaderActive]          = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        Colors[ImGuiCol_Separator]             = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
        Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.45f, 0.45f, 0.45f, 0.78f);
        Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.30f, 0.30f, 0.30f, 0.25f);
        Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.45f, 0.45f, 0.45f, 0.67f);
        Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.50f, 0.50f, 0.50f, 0.95f);
        Colors[ImGuiCol_Tab]                   = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        Colors[ImGuiCol_TabHovered]            = ImVec4(0.30f, 0.30f, 0.30f, 0.80f);
        Colors[ImGuiCol_TabActive]             = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.10f, 0.10f, 0.10f, 0.97f);
        Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        Colors[ImGuiCol_DockingPreview]        = ImVec4(0.30f, 0.30f, 0.30f, 0.70f);
        Colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        Colors[ImGuiCol_PlotLines]             = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.90f, 0.50f, 0.50f, 1.00f);
        Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.80f, 0.65f, 0.00f, 1.00f);
        Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.90f, 0.50f, 0.00f, 1.00f);
        Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.50f, 0.50f, 0.50f, 0.35f);
        Colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 0.00f, 0.00f, 0.90f);
        Colors[ImGuiCol_NavHighlight]          = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Theme::ApplyLightStyle()
    {
        ImGui::StyleColorsLight();

        Ref<ImGuiStyle> Style = ImGui::GetStyle();
        Style.WindowRounding    = 8.0f;
        Style.ChildRounding     = 8.0f;
        Style.FrameRounding     = 6.0f;
        Style.PopupRounding     = 6.0f;
        Style.ScrollbarRounding = 6.0f;
        Style.GrabRounding      = 6.0f;
        Style.TabRounding       = 6.0f;

        const Ptr<ImVec4> Colors = Style.Colors;
        Colors[ImGuiCol_Text]                  = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        Colors[ImGuiCol_TextDisabled]          = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
        Colors[ImGuiCol_WindowBg]              = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
        Colors[ImGuiCol_ChildBg]               = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
        Colors[ImGuiCol_PopupBg]               = ImVec4(0.98f, 0.98f, 0.98f, 0.98f);
        Colors[ImGuiCol_Border]                = ImVec4(0.65f, 0.65f, 0.65f, 0.70f);
        Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        Colors[ImGuiCol_FrameBg]               = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
        Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
        Colors[ImGuiCol_TitleBg]               = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.84f, 0.84f, 0.84f, 1.00f);
        Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.90f, 0.90f, 0.90f, 0.53f);
        Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.68f, 0.68f, 0.68f, 1.00f);
        Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.58f, 0.58f, 0.58f, 1.00f);
        Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.48f, 0.48f, 0.48f, 1.00f);
        Colors[ImGuiCol_CheckMark]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        Colors[ImGuiCol_SliderGrab]            = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        Colors[ImGuiCol_Button]                = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
        Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
        Colors[ImGuiCol_ButtonActive]          = ImVec4(0.66f, 0.66f, 0.66f, 1.00f);
        Colors[ImGuiCol_Header]                = ImVec4(0.78f, 0.78f, 0.78f, 0.55f);
        Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.70f, 0.70f, 0.70f, 0.80f);
        Colors[ImGuiCol_HeaderActive]          = ImVec4(0.62f, 0.62f, 0.62f, 1.00f);
        Colors[ImGuiCol_Separator]             = ImVec4(0.65f, 0.65f, 0.65f, 0.50f);
        Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.50f, 0.50f, 0.50f, 0.78f);
        Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.60f, 0.60f, 0.60f, 0.25f);
        Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.45f, 0.45f, 0.45f, 0.67f);
        Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.35f, 0.35f, 0.35f, 0.95f);
        Colors[ImGuiCol_Tab]                   = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
        Colors[ImGuiCol_TabHovered]            = ImVec4(0.76f, 0.76f, 0.76f, 0.80f);
        Colors[ImGuiCol_TabActive]             = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
        Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.92f, 0.92f, 0.92f, 0.97f);
        Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        Colors[ImGuiCol_DockingPreview]        = ImVec4(0.50f, 0.50f, 0.50f, 0.70f);
        Colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
        Colors[ImGuiCol_PlotLines]             = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.80f, 0.30f, 0.30f, 1.00f);
        Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.70f, 0.55f, 0.00f, 1.00f);
        Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.80f, 0.40f, 0.00f, 1.00f);
        Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.60f, 0.60f, 0.60f, 0.35f);
        Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.80f, 0.00f, 0.00f, 0.90f);
        Colors[ImGuiCol_NavHighlight]          = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
        Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
        Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
    }
}