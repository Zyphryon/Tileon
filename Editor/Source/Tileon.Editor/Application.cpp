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
#include "Panel/Archetypes.hpp"
#include "Panel/Viewport/Viewport.hpp"
#include "Panel/Terrains.hpp"
#include "Panel/Hierarchy.hpp"
#include "Panel/Inspector.hpp"
#include "Panel/Palette.hpp"
#include "Panel/Environment.hpp"
#include "Panel/Resources.hpp"
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

        // Load the persisted editor configuration before the engine spins up.
        const Filesystem::Path Path = Filesystem::GetDataFolder("Tileon", "Editor");
        Filesystem::Make(Path);

        Blob File;

        if (Filesystem::Read(Path + "/Config.json", File) != Filesystem::Result::Success)
        {
            return;
        }

        if (JsonValue Document = JsonDocument::Parse(Text(File.GetData<Char>(), File.GetSize())); Document.IsObject())
        {
            Startup.Load(JsonObject(Document));
        }
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

        // Point ImGui at the layout beside the editor configuration.
        ImGui::GetIO().IniFilename = nullptr;

        ImGui::LoadIniSettingsFromDisk((Filesystem::GetDataFolder("Tileon", "Editor") + "/Layout.ini").GetData());

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
            mState = State::Saving;
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
                    switch (mLauncher.Draw())
                    {
                    case Launcher::Result::Done:
                        Launch(Move(mLauncher.GetProject()));
                        break;
                    case Launcher::Result::Exit:
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
        const Filesystem::Path Path = Filesystem::GetDataFolder("Tileon", "Editor");

        JsonValue Document;
        Document.SetObject();

        Save(JsonObject(Document));
        Filesystem::Write(Path + "/Config.json", JsonDocument::Dump(Document));

        ImGui::SaveIniSettingsToDisk((Path + "/Layout.ini").GetData());

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
        mPanels.Append(Retainer<Terrains>::Create(* mContext));
        mPanels.Append(Retainer<Archetypes>::Create(* mContext));
        mPanels.Append(Retainer<Inspector>::Create(* mContext));
        mPanels.Append(Retainer<Hierarchy>::Create(* mContext));
        mPanels.Append(Retainer<Palette>::Create(* mContext));
        mPanels.Append(Retainer<Environment>::Create(* mContext));
        mPanels.Append(Retainer<Resources>::Create(* mContext));
        mPanels.Append(Retainer<Viewport>::Create(* mContext));

        // Signal that we are waiting for the content service to finish loading all queued assets.
        mState = State::Loading;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::DrawEditor(Real64 Delta)
    {
        Ref<History> History = mContext->GetHistory();

        // Retry whatever a restore had to leave waiting on a region that was still streaming in.
        History.Tick();

        // Undo reaches every panel, so it is answered here rather than inside the one that happens to hold focus.
        if (!Toolkit::Composer::IsTextInputActive())
        {
            if (Toolkit::Composer::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z))
            {
                History.Undo();
            }
            else if (Toolkit::Composer::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z)
                  || Toolkit::Composer::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y))
            {
                History.Redo();
            }
        }

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

            // Draw the "Edit" menu.
            if (Toolkit::Composer::BeginMenu("Edit"))
            {
                const String<64> Undo = History.CanUndo()
                    ? String<64>::Print<"Undo {0}">(History.GetUndoLabel()) : String<64>("Undo");
                const String<64> Redo = History.CanRedo()
                    ? String<64>::Print<"Redo {0}">(History.GetRedoLabel()) : String<64>("Redo");

                if (Toolkit::Composer::MenuItem(Undo, "Ctrl+Z", History.CanUndo()))
                {
                    History.Undo();
                }

                if (Toolkit::Composer::MenuItem(Redo, "Ctrl+Shift+Z", History.CanRedo()))
                {
                    History.Redo();
                }

                Toolkit::Composer::EndMenu();
            }

            // Draw the "View" menu.
            if (Toolkit::Composer::BeginMenu("View"))
            {
                for (ConstRetainer<Panel> Panel : mPanels)
                {
                    Bool Visibility = Panel->IsVisible();

                    if (Toolkit::Composer::Checkbox(Panel->GetTitle(), Visibility))
                    {
                        Panel->SetVisible(Visibility);
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
            const ImGuiID Bottom = Layout.Split(Center, ImGuiDir_Down, 0.25f);

            // Stack the left column vertically: Palette on top, Hierarchy below.
            ImGuiID       LeftTop    = Left;
            const ImGuiID LeftBottom = Layout.Split(LeftTop, ImGuiDir_Down, 0.5f);

            Layout.Attach("Palette",     LeftTop);
            Layout.Attach("Hierarchy",   LeftBottom);
            Layout.Attach("Inspector",   Right);
            Layout.Attach("Environment", Right);
            Layout.Attach("Scene",       Center);
            Layout.Attach("Resources",   Bottom);
        });

        // Honour a pending navigation request from another panel.
        if (const Text Target = mContext->GetString("Navigate.Panel"); !Target.IsEmpty())
        {
            for (ConstRetainer<Panel> Panel : mPanels)
            {
                if (Panel->GetTitle() == Target)
                {
                    Panel->SetVisible(true);
                    break;
                }
            }
            mContext->SetString("Navigate.Panel", Text::Empty());
        }

        // Draw each visible activity, allowing them to render their respective user interfaces.
        for (ConstRetainer<Panel> Panel : mPanels)
        {
            if (Panel->IsVisible())
            {
                Panel->OnDraw();
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Application::DrawGame(Real64 Delta)
    {
        // Render the game view to an off-screen buffer, which will be displayed in the viewport.
        if (const Ptr<ImGuiWindow> Parent = ImGui::FindWindowByName(Viewport::kTitle.GetData()); Parent && Parent->Active)
        {
            const UInt32 ViewportID   = Parent->GetID("##viewport");
            const Text   ViewportName = String<64>::Print<"{0}/##viewport_{1:08X}">(Viewport::kTitle, ViewportID);

            if (const ConstPtr<ImGuiWindow> Child = ImGui::FindWindowByName(ViewportName.GetData()); Child)
            {
                const Real32 Width  = Child->ContentSize.x;
                const Real32 Height = Child->ContentSize.y;

                if (mViewport.GetX() != Width || mViewport.GetY() != Height)
                {
                    if (Width != 0 && Height != 0)
                    {
                        mViewport.Set(Width, Height);

                        mContext->GetPresenter().Resize(Width, Height);
                    }
                }
                mContext->GetPresenter().Present(Delta);
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