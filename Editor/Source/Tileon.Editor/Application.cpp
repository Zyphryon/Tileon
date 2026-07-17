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
#include "View/Archetypes/Archetypes.hpp"
#include "View/Foundry/Foundry.hpp"
#include "View/Inspector/Inspector.hpp"
#include "View/Palette/Palette.hpp"
#include "View/Scene/Scene.hpp"
#include "Tileon.Editor.UI/Theme.hpp"
#include "Tileon_Editor.Modules.hpp"
#include <Zyphryon.Content/Mount/Disk.hpp>
#include <Zyphryon.Platform/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Application::Application()
        : mState { State::Idle }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Application::OnInitialize()
    {
        // Adds the main disk content mount for the editor, which allows loading assets from the local file system.
        ConstRetainer<Content::Service> Content = GetService<Content::Service>();
        Content->AddMount("Editor",    Retainer<Content::Disk>::Create("Editor"));
        Content->AddMount("Resources", Retainer<Content::Disk>::Create("Editor"));

        // Initialize ImGui plugin and the UI theme system, which sets up the rendering the user interface.
        mFrontend.Initialize(* this);
        UI::Theme::Initialize();

        // TODO: Manage ImGUI configuration file manually
        // TODO: Serialize the last project open.

        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::OnTick(Real64 Delta)
    {
        ConstRetainer<Graphic::Service> Graphics = GetService<Graphic::Service>();

        switch (mState)
        {
        case State::Idle:
            break;
        case State::Loading:
        {
            ConstRetainer<Content::Service> Content = GetService<Content::Service>();

            if (Content->GetPending() == 0)
            {
                mState = State::Running;
            }
            break;
        }
        case State::Running:
        {
            DrawGame();
            break;
        }
        }

        // Render the editor interface or bootstrap view depending on whether the main context has been initialized.
        ConstRef<Platform::Window> Window = GetService<Platform::Service>()->GetWindow();
        const Graphic::Viewport Viewport(0.0f, 0.0f, Window.GetWidth(), Window.GetHeight());

        Graphics->Prepare(Graphic::kDisplay, Viewport, Color::Black(), 1.0f, 0);
        {
            mFrontend.Begin(Delta);
            {
                UI::Composer Composer;

                switch (mState)
                {
                case State::Idle:
                    switch (mBootstrap.Draw(Composer))
                    {
                    case View::Bootstrap::Result::Done:
                        Launch(Move(mBootstrap.GetProject()));
                        break;
                    case View::Bootstrap::Result::Exit:
                        Quit();
                        break;
                    default:
                        break;
                    }
                    break;
                case State::Loading:
                    DrawLoading(Composer);
                    break;
                case State::Running:
                    DrawEditor(Composer, Delta);
                    break;
                }
            }
            mFrontend.End();
        }
        Graphics->Commit();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::OnTerminate()
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
        Str Directory(StrBeforeLast(Project.GetPath(), '/'));
        Directory.Append('/');

        // Add the project content mount, which allows loading assets from the project's directory.
        ConstRetainer<Content::Service> Content = GetService<Content::Service>();
        Content->AddMount("Resources", Retainer<Content::Disk>::Create(Directory));

        // Initialize the main context for the editor.
        mContext = Unique<Context>::Create(* this, Move(Project));

        // Add editor activities to the list of activities, which will be rendered in the interface.
        mActivities.Append(Retainer<View::Foundry>::Create(* mContext));
        mActivities.Append(Retainer<View::Archetypes>::Create(* mContext));
        mActivities.Append(Retainer<View::Inspector>::Create(* mContext));
        mActivities.Append(Retainer<View::Palette>::Create(* mContext));
        mActivities.Append(Retainer<View::Scene>::Create(* mContext));

        // Signal that we are waiting for the content service to finish loading all queued assets.
        mState = State::Loading;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::DrawEditor(Ref<UI::Composer> Composer, Real64 Delta)
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
                    Quit();
                }

                Composer.EndMenu();
            }

            // Draw the "View" menu.
            if (Composer.BeginMenu("View"))
            {
                for (ConstRetainer<Activity> Activity : mActivities)
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
        for (ConstRetainer<Activity> Activity : mActivities)
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

    void Application::DrawGame()
    {
        // Render the game view to an off-screen buffer, which will be displayed in the scene activity's viewport.
        const Ptr<ImGuiWindow> Parent = ImGui::FindWindowByName(View::Scene::kTitle.GetData());

        if (Parent && Parent->Active)
        {
            const UInt32 ViewportID   = Parent->GetID("##viewport");
            const Text   ViewportName = String<64>::Print<"{0}/##viewport_{1:08X}">(View::Scene::kTitle, ViewportID);

            if (const ConstPtr<ImGuiWindow> Child = ImGui::FindWindowByName(ViewportName.GetData()); Child)
            {
                const Real32 Width  = Child->ContentSize.x;
                const Real32 Height = Child->ContentSize.y;

                if (mViewport.GetX() != Width || mViewport.GetY() != Height)
                {
                    if (Width != 0 && Height != 0)
                    {
                        mViewport.Set(Width, Height);

                        mContext->GetController().Resize(Width, Height);
                    }
                }
                mContext->GetController().Present();
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::DrawLoading(Ref<UI::Composer> Composer)
    {
        ConstRetainer<Content::Service> Content = GetService<Content::Service>();

        // Center a fixed-size, loading window.
        ImGui::SetNextWindowSize(ImVec2(300, 70), ImGuiCond_Always);
        Composer.SetNextWindowPos(Composer.GetViewportCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowBgAlpha(0.88f);

        constexpr ImGuiWindowFlags kFlags =
            ImGuiWindowFlags_NoDecoration          |
            ImGuiWindowFlags_NoInputs              |
            ImGuiWindowFlags_NoNav                 |
            ImGuiWindowFlags_NoMove                |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoDocking;

        if (ImGui::Begin("##Loading", nullptr, kFlags))
        {
            // Animated ellipsis based on elapsed time.
            const UInt32 Dots  = static_cast<UInt32>(ImGui::GetTime() * 3.0) % 4;
            const UInt32 Count = Content->GetPending();

            auto Ellipsis = Dots == 0 ? "" : Dots == 1 ? "." : Dots == 2 ? ".." : "...";

            const Text Label = String<128>::Print<"Loading{0} ({1} asset{2} remaining)">(Ellipsis, Count, Count == 1u ? "" : "s");

            // Center the text inside the window.
            const ImVec2 Available = ImGui::GetContentRegionAvail();
            const ImVec2 TextSize  = ImGui::CalcTextSize(Label.GetData());
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (Available.x - TextSize.x) * 0.5f);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (Available.y - TextSize.y) * 0.5f);
            ImGui::TextUnformatted(Label.GetData());
        }
        ImGui::End();
    }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   MAIN   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

int main([[maybe_unused]] int Argc, [[maybe_unused]] Ptr<Char> Argv[])
{
    // Load the editor configuration from the user's home directory "Tileon/Editor/Config.toml".
    //Blob Configuration;
    //Filesystem::Read(Filesystem::GetDataFolder("Tileon", "Editor") + "Config.toml", Configuration);

    //JsonValue Archive = JsonDocument::Parse(Text(Configuration.GetData<Char>(), Configuration.GetSize()));

    Engine::Config Properties;
    Properties.SetWindowTitle("Tileon Editor (v0.1)");

#if   defined(ZY_PLATFORM_WINDOWS)
    Properties.SetGraphicsDriver("D3D11");
#else
    Properties.SetGraphicsDriver("GLES3");
#endif

   // Properties.Load(Archive);

    // Initialize 'Zyphryon Engine' and enter main loop.
    const Unique<Tileon::Editor::Application> Application = Unique<Tileon::Editor::Application>::Create();
    Application->Run(Move(Properties), ZyRegisterModules());
   // Application->Sync(Properties);
    // TODO
    // Save the editor configuration back to the user's home directory.
    //Properties.Save(Archive);

    //Filesystem::Write(Filesystem::GetDataFolder("Tileon", "Editor") + "Config.toml", Archive.Dump());

    return 0;
}