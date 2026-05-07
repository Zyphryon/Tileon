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
#include "Tileon.Editor.UI/Theme.hpp"
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
        // Adds the main disk content mount for the editor, which allows loading assets from the local file system.
        ConstTracker<Content::Service> Content = GetService<Content::Service>();
        Content->AddMount("Editor", Tracker<Content::Disk>::Create("Editor"));

        // Initialize ImGui plugin and the UI theme system, which sets up the rendering the user interface.
        mFrontend.Initialize(* this, GetDevice());
        UI::Theme::Initialize();

        // TODO: Serialize the last project open.

        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::OnTick(Time Time)
    {
        ConstTracker<Graphic::Service> Graphics = GetService<Graphic::Service>();

        // Render the game view to an off-screen buffer, which will be displayed in the scene activity's viewport.
        if (mContext)
        {
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
        }

        // Render the editor interface or bootstrap view depending on whether the main context has been initialized.
        const Graphic::Viewport Viewport(0.0f, 0.0f, GetDevice().GetWidth(), GetDevice().GetHeight());

        Graphics->Prepare(Graphic::kDisplay, Viewport, Color::Black(), 1.0f, 0);
        {
            mFrontend.Begin(Time);
            {
                UI::Composer Composer;

                if (mContext)
                {
                    DrawEditor(Composer, Time);
                }
                else
                {
                    const View::Bootstrap::Result Result = mBootstrap.Draw(Composer, GetDevice());

                    switch (Result)
                    {
                    case View::Bootstrap::Result::Done:
                        Launch(Move(mBootstrap.GetProject()));
                        break;
                    case View::Bootstrap::Result::Exit:
                        Exit();
                        break;
                    default:
                        break;
                    }
                }
            }
            mFrontend.End();
        }
        Graphics->Commit();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::OnTeardown()
    {
        if (mContext)
        {
            mContext->Teardown();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::Launch(AnyRef<Project> Project)
    {
        const Str8 Directory(Project.GetPath().substr(0, Project.GetPath().find_last_of("/\\") + 1));

        // Add the project content mount, which allows loading assets from the project's directory.
        ConstTracker<Content::Service> Content = GetService<Content::Service>();
        Content->AddMount("Resources", Tracker<Content::Disk>::Create(Directory));

        // Initialize the main context for the editor.
        mContext = Unique<Context>::Create(* this, Move(Project));
        mContext->GetTileset().Preload();

        // Add editor activities to the list of activities, which will be rendered in the interface.
        mActivities.push_back(Tracker<View::Browser>::Create(* mContext));
        mActivities.push_back(Tracker<View::Inspector>::Create(* mContext));
        mActivities.push_back(Tracker<View::Scene>::Create(* mContext));
        mActivities.push_back(Tracker<View::Palette>::Create(* mContext));

        // Wait until the content service is fully initialized
        // TODO: show a loading screen instead of blocking the main thread
        Content->Wait();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::DrawEditor(Ref<UI::Composer> Composer, Time Time)
    {
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
                        UI::Theme::ApplyDarkStyle();
                    }
                    if (Composer.MenuItem("Light"))
                    {
                        UI::Theme::ApplyLightStyle();
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
    // Load the editor configuration from the user's home directory "Tileon/Editor/Config.toml".
    const Blob Configuration = Filesystem::Load(Filesystem::GetHome("Tileon", "Editor") + "Config.toml");
    TOMLParser Parser(Configuration.GetText());

    Engine::Properties Properties;
    Properties.SetWindowTitle("Tileon Editor (v0.1)");
    Properties.SetVideoDriver("D3D11");
    Properties.Load(Parser);

    // Initialize 'Zyphryon Engine' and enter main loop.
    const Unique<Tileon::Editor::Application> Application = Unique<Tileon::Editor::Application>::Create();
    Application->Initialize(Service::Host::Mode::Client, Move(Properties));
    Application->Run();
    Application->Sync(Properties);

    // Save the editor configuration back to the user's home directory.
    Properties.Save(Parser);
    Filesystem::Save(Filesystem::GetHome("Tileon", "Editor") + "Config.toml", Parser.Dump());

    return 0;
}