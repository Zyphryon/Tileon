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

#include "Application.hpp"
#include "View/Browser/Browser.hpp"
#include "View/Inspector/Inspector.hpp"
#include "View/Palette/Palette.hpp"
#include "View/Scene/Scene.hpp"
#include "Tileon.World/Component/Condition/Lifecycle.hpp"
#include <Zyphryon.Content/Mount/Disk.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Application::OnInitialize()
    {
        // Set up resource management system and asset path.
        /// TODO: Project Management (+ Configuration)
        ConstTracker<Content::Service> Content = GetService<Content::Service>();
        Content->AddMount("Resources", Tracker<Content::Disk>::Create("Resources"));

        // Initialize the ImGui frontend for rendering the user interface.
        mFrontend.Initialize(* this, GetDevice());
        InitializeTheme();

        // Create the main context for the editor, which provides access to various services.
        // TODO: Project Management (+ Configuration)
        Profile Configuration;
        Configuration.SetDisplay(GetDevice().GetWidth(), GetDevice().GetHeight(), 32.0f);

        mContext = Unique<Context>::Create(* this, Configuration);

        // Add editor activities to the list of activities, which will be rendered in the interface.
        mActivities.push_back(Tracker<View::Browser>::Create(* mContext));
        mActivities.push_back(Tracker<View::Inspector>::Create(* mContext));
        mActivities.push_back(Tracker<View::Scene>::Create(* mContext));
        mActivities.push_back(Tracker<View::Palette>::Create(* mContext));

        // Preload the tileset to ensure that all necessary resources are available before the editor starts.
        mContext->GetTileset().Preload();

        // Set up an observer to automatically add the Persist component to any region that is loaded,
        // ensuring that changes to the region are saved.
        ConstTracker<Scene::Service> Scene = GetService<Scene::Service>();
        Scene->CreateObserver<Scene::DSL::In<Region>>("Editor::OnRegionLoadMakePersist", EcsOnSet,
            [](Scene::Entity Actor, Ref<Region> Region)
            {
                Actor.Add<Persist>();
            });

        // Wait for all content to finish loading before allowing the editor to run.
        Content->Wait();
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::OnTick(Time Time)
    {
        ConstTracker<Graphic::Service> Graphics = GetService<Graphic::Service>();

        // Render the game world to the offscreen texture first so that ImGui picks up
        // the current frame's content (not the previous frame's).
        const Ptr<ImGuiWindow> Parent = ImGui::FindWindowByName(View::Scene::kTitle);

        if (Parent && Parent->Active)
        {
            const UInt32    ViewportID   = Parent->GetID("##viewport");
            const ConstStr8 ViewportName = Format("{}/##viewport_{:08X}", View::Scene::kTitle, ViewportID);

            if (const ConstPtr<ImGuiWindow> Child = ImGui::FindWindowByName(ViewportName.data()); Child)
            {
                DrawGame(Child->ContentSize.x, Child->ContentSize.y);
            }
        }

        const Graphic::Viewport Viewport(0.0f, 0.0f, GetDevice().GetWidth(), GetDevice().GetHeight());

        Graphics->Prepare(Graphic::kDisplay, Viewport, Color::Black(), 1.0f, 0);
        {
            mFrontend.Begin(Time);
            {
                DrawInterface(Time);
            }
            mFrontend.End();
        }
        Graphics->Commit();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::OnTeardown()
    {
        mContext->Teardown();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::InitializeTheme()
    {
        // Configure font settings for the user interface.
        ImFontConfig Config;
        Config.MergeMode        = true;
        Config.PixelSnapH       = true;
        Config.GlyphMinAdvanceX = 13.0f;

        constexpr ImWchar kIconRange[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

        Ptr<ImFontAtlas> Atlas = ImGui::GetIO().Fonts;
        Atlas->Clear();
        Atlas->AddFontFromFileTTF("Editor\\Fonts\\Roboto-Medium.ttf",  16.0f);
        Atlas->AddFontFromFileTTF("Editor\\Fonts\\fa-regular-400.ttf", 16.0f, &Config, kIconRange);
        Atlas->AddFontFromFileTTF("Editor\\Fonts\\fa-solid-900.ttf",   16.0f, &Config, kIconRange);
        Atlas->Build();

        // Allow moving windows by dragging only the title bar, preventing unwanted window movement.
        ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

        // Apply the dark theme to the user interface by default.
        SetDarkTheme();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::SetDarkTheme()
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

    void Application::SetLightTheme()
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

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::DrawInterface(Time Time)
    {
        UI::Composer Composer;

        // Draw the main menu bar at the top.
        if (Composer.BeginMainMenuBar())
        {
            // Draw the "File" menu.
            if (Composer.BeginMenu("File"))
            {
                if (Composer.MenuItem("Save", "Ctrl+S"))
                {
                    mContext->GetController().Save();
                }

                Composer.Separator();

                if (Composer.MenuItem("Exit"))
                {
                    Exit();
                }

                Composer.EndMenu();
            }

            // Draw the "View" menu.
            if (Composer.BeginMenu("View"))
            {
                for (ConstTracker<Activity> Activity : mActivities)
                {
                    Bool Visibility = Activity->IsVisible();

                    if (Composer.Checkbox(Activity->GetTitle(), Visibility))
                    {
                        Activity->SetVisible(Visibility);
                    }
                }

                Composer.EndMenu();
            }

            // Draw the "Help" menu.
            if (Composer.BeginMenu("Help"))
            {
                if (Composer.BeginMenu("Theme"))
                {
                    if (Composer.MenuItem("Dark"))
                    {
                        SetDarkTheme();
                    }
                    if (Composer.MenuItem("Light"))
                    {
                        SetLightTheme();
                    }

                    Composer.EndMenu();
                }

                Composer.EndMenu();
            }

            Composer.EndMainMenuBar();
        }

        // Draw each visible activity, allowing them to render their respective user interfaces.
        for (ConstTracker<Activity> Activity : mActivities)
        {
            if (Activity->IsVisible())
            {
                Activity->OnDraw(Composer);
            }
        }

        // TODO: Draw bottom bar
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::DrawGame(UInt16 Width, UInt16 Height)
    {
        if (mViewport.GetX() != Width || mViewport.GetY() != Height)
        {
            if (Width != 0 && Height != 0)
            {
                mViewport.Set(Width, Height);

                mContext->GetController().Resize(Width, Height);
            }
        }
        mContext->GetController().Present(false);
    }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   MAIN   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

int main([[maybe_unused]] int Argc, [[maybe_unused]] Ptr<Char> Argv[])
{
    Engine::Properties Properties;
    Properties.SetWindowTitle("Tileon's Editor");
    Properties.SetWindowWidth(1280);
    Properties.SetWindowHeight(768);
    Properties.SetWindowSamples(1);
    Properties.SetWindowFullscreen(false);
    Properties.SetWindowBorderless(false);
    Properties.SetVideoDriver("D3D11");

    // Initialize 'Zyphryon Engine' and enter main loop.
    const Unique<Tileon::Editor::Application> Application = Unique<Tileon::Editor::Application>::Create();
    Application->Initialize(Service::Host::Mode::Client, Move(Properties));
    Application->Run();

    return 0;
}