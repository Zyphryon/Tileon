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
#include <Zyphryon.Content/Mount/Disk.hpp>
#include <Zyphryon.Content/Service.hpp>
#include <Zyphryon.Graphic/Service.hpp>

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
        Content->AddMount("Resources", Tracker<Content::Disk>::Create("Resources"));

        // Initialize the ImGui frontend for rendering the user interface.
        mFrontend.Initialize(* this, GetDevice());

        // Create the main context for the editor, which provides access to various services.
        mContext = Unique<Context>::Create(* this);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::OnTick(Time Time)
    {
        ConstTracker<Graphic::Service> Graphics = GetService<Graphic::Service>();

        const Graphic::Viewport Viewport(0.0f, 0.0f, GetDevice().GetWidth(), GetDevice().GetHeight());

        Graphics->Prepare(Graphic::kDisplay, Viewport, Graphic::Clear::All, Color::Black(), 1.0f, 0);
        {
            mFrontend.Begin(Time);
            {
                DrawInterface(Time);
            }
            mFrontend.End();
        }
        Graphics->Commit(Graphic::kDisplay);

        // Draw the game-view in a separate pass to ensure that it is rendered inside a interface window.
        DrawGame(Time);
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
        // Draw the main menu bar at the top of the interface.
        if (ImGui::BeginMainMenuBar())
        {
            // Draw the "File" menu.
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    mContext->GetController().Save();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Exit"))
                {
                    Exit();
                }

                ImGui::EndMenu();
            }

            // Draw the "View" menu.
            if (ImGui::BeginMenu("View"))
            {
                for (ConstTracker<Activity> Activity : mActivities)
                {
                    Bool Visibility = Activity->IsVisible();

                    if (mPresenter.Checkbox(Activity->GetTitle(), Visibility))
                    {
                        Activity->SetVisible(Visibility);
                    }
                }

                ImGui::EndMenu();
            }

            // Draw the "Help" menu.
            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("Reset", "Ctrl+Alt+R"))
                {
                    // TODO: Reset the editor to its default state
                }

                if (ImGui::MenuItem("About"))
                {
                    // TODO: Show an about dialog with information about the editor
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        // TODO: Content Browser
        // TODO: Viewport
        // TODO: Hierarchy Browser
        // TODO: Inspector
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::DrawGame(Time Time)
    {
        // TODO: Draw
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