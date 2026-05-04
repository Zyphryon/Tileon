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
#include "View/Scene/Scene.hpp"
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
        ConstTracker<Content::Service> Content = GetService<Content::Service>();
        Content->AddMount("Resources", Tracker<Content::Disk>::Create("C:\\Users\\Wolftein\\Workspace\\Aurora-Game-Resources"));

        // Initialize the ImGui frontend for rendering the user interface.
        mFrontend.Initialize(* this, GetDevice());

        // Create the main context for the editor, which provides access to various services.
        Profile Configuration;
        Configuration.SetDisplay(GetDevice().GetWidth(), GetDevice().GetHeight(), 32.0f); // TODO: Configurable

        mContext = Unique<Context>::Create(* this, Configuration);

        // Add editor activities to the list of activities, which will be rendered in the interface.
        mActivities.push_back(Tracker<View::Browser>::Create(* mContext));
        mActivities.push_back(Tracker<View::Inspector>::Create(* mContext));
        mActivities.push_back(Tracker<View::Scene>::Create(* mContext));

        // Wait for all content to finish loading before allowing the editor to run.
        Content->Wait();
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::OnTick(Time Time)
    {
        ConstTracker<Graphic::Service> Graphics = GetService<Graphic::Service>();

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

        // Draw the game-view in a separate pass to ensure that it is rendered inside a interface window.
        const ConstPtr<ImGuiWindow> Window = ImGui::FindWindowByName(View::Scene::kTitle);

        if (Window && Window->Active)
        {
            DrawGame(static_cast<UInt16>(Window->ContentSize.x), static_cast<UInt16>(Window->ContentSize.y));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::OnTeardown()
    {
        mContext->Teardown();
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
                if (Composer.MenuItem("Reset", "Ctrl+Alt+R"))
                {
                    // TODO: Reset the editor to its default state
                }

                if (Composer.MenuItem("About"))
                {
                    // TODO: Show an about dialog with information about the editor
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
        static IntVector2 LastViewportSize = { 0, 0 };

        if (LastViewportSize.GetX() != Width && LastViewportSize.GetY() != Height)
        {
            if (Width != 0 && Height != 0)
            {
                LastViewportSize.Set(Width, Height);

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