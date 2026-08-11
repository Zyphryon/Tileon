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
#include "Activity/Archetypes/Archetypes.hpp"
#include "Activity/Atelier/Atelier.hpp"
#include "Activity/Foundry/Foundry.hpp"
#include "Activity/Hierarchy/Hierarchy.hpp"
#include "Activity/Inspector/Inspector.hpp"
#include "Activity/Palette/Palette.hpp"
#include "Activity/Universe/Universe.hpp"
#include "Tileon.Editor/Toolkit/Theme.hpp"
#include "Tileon_Editor.Modules.hpp"
#include "Tileon_Editor.Embedded.hpp"
#include <Zyphryon.Content/Mount/Disk.hpp>
#include <Zyphryon.Platform/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void LoadConfig(Ref<Runtime::Startup> Config, ConstRef<Filesystem::Path> Path)
    {
        Blob File;

        if (Filesystem::Read(Path, File) != Filesystem::Result::Success)
        {
            return;
        }

        if (JsonValue Document = JsonDocument::Parse(Text(File.GetData<Char>(), File.GetSize())); Document.IsObject())
        {
            const JsonObject Root(Document);

            if (const JsonObject Section = Root.GetObject("Window"); Section.IsValid())
            {
                Config.SetWindowMonitor(Section.GetString("monitor", Config.GetWindowMonitor()));
                Config.SetWindowWidth(Section.GetNumber<UInt32>("width", Config.GetWindowWidth()));
                Config.SetWindowHeight(Section.GetNumber<UInt32>("height", Config.GetWindowHeight()));
                Config.SetWindowFullscreen(Section.GetBool("fullscreen", Config.IsWindowFullscreen()));
            }

            if (const JsonObject Section = Root.GetObject("Graphic"); Section.IsValid())
            {
                Config.SetGraphicsTearless(Section.GetBool("tearless", Config.IsGraphicsTearless()));
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void SyncConfig(Ref<Runtime::Startup> Config, Ref<Engine::Subsystem::Host> Host)
    {
        ConstRetainer<Platform::Service> Platform = Host.GetService<Platform::Service>();

        ConstRef<Platform::Window> Window = Platform->GetWindow();
        Config.SetWindowWidth(Window.GetWidth());
        Config.SetWindowHeight(Window.GetHeight());
        Config.SetWindowFullscreen(Window.IsFullscreen());

        if (const ConstPtr<Platform::Monitor> Monitor = Platform->GetDisplay().GetMonitor(Window.GetX(), Window.GetY()))
        {
            Config.SetWindowMonitor(Monitor->GetName());
        }

        ConstRetainer<Graphic::Service> Graphics = Host.GetService<Graphic::Service>();
        Config.SetGraphicsTearless(Graphics->IsTearless());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void SaveConfig(Ref<Runtime::Startup> Config, ConstRef<Filesystem::Path> Path)
    {
        JsonValue Document;
        Document.SetObject();

        JsonObject Root(Document);

        JsonObject Window = Root.SetObject("Window");
        Window.SetString("monitor", Config.GetWindowMonitor());
        Window.SetNumber("width", Config.GetWindowWidth());
        Window.SetNumber("height", Config.GetWindowHeight());
        Window.SetBool("fullscreen", Config.IsWindowFullscreen());

        JsonObject Graphic = Root.SetObject("Graphic");
        Graphic.SetBool("tearless", Config.IsGraphicsTearless());

        const Str Data = JsonDocument::Dump(Document);
        Filesystem::Write(Path, ConstSpan(reinterpret_cast<ConstPtr<Byte>>(Data.GetData()), Data.GetSize()));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Application::Application()
        : mState { State::Idle }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::OnConfigure(Ref<Runtime::Startup> Startup)
    {
        Startup.SetWindowTitle("Tileon Editor (v0.1)");
        Startup.SetGraphicsColorFormat(Graphic::TextureFormat::RGBA8UIntNorm_sRGB);

#if   defined(ZY_PLATFORM_WEB)
        Startup.SetWindowBorderless(true);
#endif

#if   defined(ZY_PLATFORM_WINDOWS)
        Startup.SetGraphicsDriver("D3D11");
#elif defined(ZY_PLATFORM_WEB) || defined(ZY_PLATFORM_LINUX)
        Startup.SetGraphicsDriver("GLES3");
#endif

        // Load the persisted editor configuration before the engine spins up.
        const Filesystem::Path Path = Filesystem::GetDataFolder("Tileon", "Editor");
        Filesystem::Make(Path);
        LoadConfig(Startup, Path + "/Config.json");
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Application::OnInitialize()
    {
        // Adds the main disk content mount for the editor, which allows loading assets from the local file system.
        ConstRetainer<Content::Service> Content = GetService<Content::Service>();
        ZyRegisterEmbedded(* Content);
        Content->AddMount("Resources", Retainer<Content::Disk>::Create("Editor"));

        // Initialize ImGui plugin and the UI theme system, which sets up the rendering the user interface.
        mFrontend.Initialize(* this, Plugin::Colorspace::sRGB);
        Toolkit::Theme::Initialize(* Content);

        // TODO: Manage ImGUI configuration file manually
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
        case State::Preparing:
        {
            // A bake refuses to write while a sheet it cuts from is still arriving, so they are awaited first.
            if (mContext->Prepare())
            {
                mState = State::Saving;
            }
            break;
        }
        case State::Saving:
        {
            mContext->Commit();

            mState = State::Running;
            break;
        }
        case State::Running:
        {
            DrawGame(Delta);
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
                switch (mState)
                {
                case State::Idle:
                    switch (mBootstrap.Draw())
                    {
                    case Bootstrap::Result::Done:
                        Launch(Move(mBootstrap.GetProject()));
                        break;
                    case Bootstrap::Result::Exit:
                        Quit();
                        break;
                    default:
                        break;
                    }
                    break;
                case State::Loading:
                    DrawLoading();
                    break;
                case State::Preparing:
                case State::Saving:
                    DrawEditor(Delta);
                    DrawNotice("##Saving", "Saving project...");
                    break;
                case State::Running:
                    DrawEditor(Delta);
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
        SyncConfig(mStartup, * this);
        SaveConfig(mStartup, Filesystem::GetDataFolder("Tileon", "Editor") + "/Config.json");

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
        mActivities.Append(Retainer<Foundry>::Create(* mContext));
        mActivities.Append(Retainer<Archetypes>::Create(* mContext));
        mActivities.Append(Retainer<Inspector>::Create(* mContext));
        mActivities.Append(Retainer<Hierarchy>::Create(* mContext));
        mActivities.Append(Retainer<Palette>::Create(* mContext));
        mActivities.Append(Retainer<Universe>::Create(* mContext));
        mActivities.Append(Retainer<Atelier>::Create(* mContext));

        // Signal that we are waiting for the content service to finish loading all queued assets.
        mState = State::Loading;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::DrawEditor(Real64 Delta)
    {
        // Draw the main menu bar at the top.
        if (Toolkit::Composer::BeginMainMenuBar())
        {
            // Draw the "File" menu.
            if (Toolkit::Composer::BeginMenu("File"))
            {
                if (Toolkit::Composer::MenuItem("Save", "Ctrl+S"))
                {
                    mState = State::Preparing;
                }

                Toolkit::Composer::Separator();

                if (Toolkit::Composer::MenuItem("Exit"))
                {
                    Quit();
                }

                Toolkit::Composer::EndMenu();
            }

            // Draw the "View" menu.
            if (Toolkit::Composer::BeginMenu("View"))
            {
                for (ConstRetainer<Activity> Activity : mActivities)
                {
                    Bool Visibility = Activity->IsVisible();

                    if (Toolkit::Composer::Checkbox(Activity->GetTitle(), Visibility))
                    {
                        Activity->SetVisible(Visibility);
                    }
                }

                Toolkit::Composer::Separator();

                // Discarding the node makes the dockspace below rebuild the default arrangement next frame.
                if (Toolkit::Composer::MenuItem("Reset Layout"))
                {
                    Toolkit::Composer::ResetDockSpace("EditorDockSpace");
                }

                Toolkit::Composer::EndMenu();
            }

            // Draw the "Settings" menu.
            if (Toolkit::Composer::BeginMenu("Settings"))
            {
                Ref<Platform::Window> Window   = GetService<Platform::Service>()->GetWindow();

                Bool Fullscreen = Window.IsFullscreen();

                if (Toolkit::Composer::Checkbox("Fullscreen", Fullscreen))
                {
                    Window.SetFullscreen(Fullscreen);
                }

                ConstRetainer<Graphic::Service> Graphics = GetService<Graphic::Service>();

                Bool Tearless = Graphics->IsTearless();

                if (Toolkit::Composer::Checkbox("Tearless", Tearless))
                {
                    Graphics->Reset(Window.GetWidth(), Window.GetHeight(), Tearless);
                }

                Toolkit::Composer::EndMenu();
            }

            // Draw the "Help" menu.
            if (Toolkit::Composer::BeginMenu("Help"))
            {
                if (Toolkit::Composer::BeginMenu("Theme"))
                {
                    if (Toolkit::Composer::MenuItem("Dark"))
                    {
                        Toolkit::Theme::ApplyDarkStyle();
                    }
                    if (Toolkit::Composer::MenuItem("Light"))
                    {
                        Toolkit::Theme::ApplyLightStyle();
                    }

                    Toolkit::Composer::EndMenu();
                }

                Toolkit::Composer::EndMenu();
            }

            Toolkit::Composer::EndMainMenuBar();
        }

        // Host dockspace filling the viewport's work area; the builder only runs on the first run.
        Toolkit::Composer::DockSpace("EditorDockSpace", [](Ref<Toolkit::Dock> Layout)
        {
            ImGuiID       Center = Layout.GetRoot();
            const ImGuiID Left   = Layout.Split(Center, ImGuiDir_Left,  0.20f);
            const ImGuiID Right  = Layout.Split(Center, ImGuiDir_Right, 0.25f);

            // Stack the left column vertically: Palette on top, Hierarchy below.
            ImGuiID       LeftTop    = Left;
            const ImGuiID LeftBottom = Layout.Split(LeftTop, ImGuiDir_Down, 0.5f);

            Layout.Attach("Palette",   LeftTop);
            Layout.Attach("Hierarchy", LeftBottom);
            Layout.Attach("Inspector", Right);
            Layout.Attach("Universe",  Right);
            Layout.Attach("Scene",     Center);
        });

        // Honour a pending navigation request from another panel.
        if (const Text Target = mContext->GetString("Navigate.Panel"); !Target.IsEmpty())
        {
            for (ConstRetainer<Activity> Activity : mActivities)
            {
                if (Activity->GetTitle() == Target)
                {
                    Activity->SetVisible(true);
                    break;
                }
            }
            mContext->SetString("Navigate.Panel", Text::Empty());
        }

        // Draw each visible activity, allowing them to render their respective user interfaces.
        for (ConstRetainer<Activity> Activity : mActivities)
        {
            if (Activity->IsVisible())
            {
                Activity->OnDraw();
            }
        }

        // TODO: Draw bottom bar
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::DrawGame(Real64 Delta)
    {
        // A motif authored since the last bake has no frames on the GPU, so the editor copies its own.
        mContext->GetForge().Tick();

        // Render the game view to an off-screen buffer, which will be displayed in the atelier activity's viewport.
        if (const Ptr<ImGuiWindow> Parent = ImGui::FindWindowByName(Atelier::kTitle.GetData()); Parent && Parent->Active)
        {
            const UInt32 ViewportID   = Parent->GetID("##viewport");
            const Text   ViewportName = String<64>::Print<"{0}/##viewport_{1:08X}">(Atelier::kTitle, ViewportID);

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
                mContext->GetController().Present(Delta);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::DrawLoading()
    {
        ConstRetainer<Content::Service> Content = GetService<Content::Service>();

        // Animated ellipsis based on elapsed time.
        const UInt32 Dots  = static_cast<UInt32>(Toolkit::Composer::GetTime() * 3.0) % 4;
        const UInt32 Count = Content->GetPending();

        auto Ellipsis = Dots == 0 ? "" : Dots == 1 ? "." : Dots == 2 ? ".." : "...";

        const String<128> Label = String<128>::Print<"Loading{0} ({1} asset{2} remaining)">(
            Ellipsis, Count, Count == 1u ? "" : "s");

        DrawNotice("##Loading", Label);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::DrawNotice(Text Identifier, Text Label)
    {
        Toolkit::Composer::SetNextWindowSize(300.0f, 70.0f, ImGuiCond_Always);
        Toolkit::Composer::SetNextWindowPos(Toolkit::Composer::GetViewportCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        Toolkit::Composer::SetNextWindowBgAlpha(0.88f);

        constexpr ImGuiWindowFlags kFlags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoInputs     |
            ImGuiWindowFlags_NoNav        |
            ImGuiWindowFlags_NoMove       |
            ImGuiWindowFlags_NoDocking;

        if (Toolkit::Composer::Begin(Identifier, kFlags))
        {
            const ImVec2 Available = Toolkit::Composer::GetContentRegionAvail();
            const ImVec2 TextSize  = Toolkit::Composer::CalcTextSize(Label);
            Toolkit::Composer::BringWindowToFront();
            Toolkit::Composer::SetCursorPosX(Toolkit::Composer::GetCursorPosX() + (Available.x - TextSize.x) * 0.5f);
            Toolkit::Composer::SetCursorPosY(Toolkit::Composer::GetCursorPosY() + (Available.y - TextSize.y) * 0.5f);
            Toolkit::Composer::TextUnformatted(Label);
        }
        Toolkit::Composer::End();
    }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   MAIN   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

ZY_APPLICATION(Tileon::Editor::Application)