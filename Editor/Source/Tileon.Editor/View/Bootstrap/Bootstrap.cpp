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

#include "Bootstrap.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bootstrap::Bootstrap()
        : mState { State::Menu }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bootstrap::Result Bootstrap::Draw(Ref<UI::Composer> Composer, ConstRef<Engine::Device> Device)
    {
        if (mState == State::Done)
        {
            return Result::Done;
        }

        // Draw a solid background to cover the entire display area.
        ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), Composer.GetDisplaySize(), IM_COL32(18, 18, 28, 255));

        switch (mState)
        {
        case State::Menu:
            ImGui::SetNextWindowSize(ImVec2(240, 170));
            break;
        case State::Wizard:
            ImGui::SetNextWindowSize(ImVec2(400, 240));
            break;
        default:
            break;
        }
        Composer.SetNextWindowPos(Composer.GetViewportCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        constexpr ImGuiWindowFlags kWindowFlags =
            ImGuiWindowFlags_NoResize              |
            ImGuiWindowFlags_NoMove                |
            ImGuiWindowFlags_NoCollapse            |
            ImGuiWindowFlags_NoScrollbar           |
            ImGuiWindowFlags_NoNavFocus            |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoDocking             |
            ImGuiWindowFlags_NoTitleBar            |
            ImGuiWindowFlags_NoDecoration;

        Result Result = Result::None;

        if (Composer.Begin("##bootstrap", kWindowFlags))
        {
            Composer.Spacing();

            switch (mState)
            {
            case State::Menu:
                Result = DrawInMenu(Composer, Device);
                break;
            case State::Wizard:
                Result = DrawInWizard(Composer, Device);
                break;
            default:
                break;
            }

            Composer.End();
        }

        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bootstrap::Result Bootstrap::DrawInMenu(Ref<UI::Composer> Composer, ConstRef<Engine::Device> Device)
    {
        constexpr Real32 kButtonW = 160.0f;
        constexpr Real32 kButtonH = 32.0f;
        const Real32     kOffsetX = (Composer.GetWindowWidth() - kButtonW) * 0.5f;

        Composer.SetCursorPosX(kOffsetX);
        if (Composer.Button("Open", kButtonW, kButtonH))
        {
            OpenFileDialog(Device);
        }

        Composer.Spacing();

        Composer.SetCursorPosX(kOffsetX);
        if (Composer.Button("Create", kButtonW, kButtonH))
        {
            mState = State::Wizard;
        }

        Composer.SetCursorPosY(Composer.GetWindowBottom(2));
        Composer.Separator();
        Composer.Spacing();

        Composer.SetCursorPosX(kOffsetX);
        if (Composer.Button("Exit", kButtonW, kButtonH))
        {
            return Result::Exit;
        }
        return Result::None;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bootstrap::Result Bootstrap::DrawInWizard(Ref<UI::Composer> Composer, ConstRef<Engine::Device> Device)
    {
        constexpr Real32 kLabelW  = 90.0f;
        constexpr Real32 kButtonW = 80.0f;

        Composer.Label("Name");
        Composer.SameLine(kLabelW);
        Composer.SetNextItemWidth(-1.0f);
        Composer.InputText("##name", mProject.GetName(), [this](ConstStr8 Value)
        {
            mProject.SetName(Value);
        });

        constexpr Array<UInt16, 6> kDensityOptions = { 8, 16, 32, 64, 128, 256 };

        Composer.Label("Density");
        Composer.SameLine(kLabelW);
        Composer.SetNextItemWidth(-1.0f);
        if (Composer.BeginCombo("##density", Format("{}", mProject.GetDensity())))
        {
            for (const UInt16 Option : kDensityOptions)
            {
                const Bool Selected = (mProject.GetDensity() == Option);

                if (Composer.Selectable(Format("{}", Option), Selected))
                {
                    mProject.SetDensity(Option);
                }
            }
            Composer.EndCombo();
        }

        Composer.Label("Description");
        Composer.SameLine(kLabelW);
        Composer.SetNextItemWidth(-1.0f);
        Composer.InputText("##description", mProject.GetDescription(), [this](ConstStr8 Value)
        {
            mProject.SetDescription(Value);
        });

        Composer.SetCursorPosY(Composer.GetWindowBottom());

        if (Composer.Button("Back", kButtonW))
        {
            mState = State::Menu;
        }

        Composer.SameLine(Composer.GetWindowWidth() - kButtonW - Composer.GetStyle().WindowPadding.x);

        if (Composer.Button("Create", kButtonW))
        {
            OpenSaveDialog(Device);
        }

        return Result::None;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Bootstrap::OpenFileDialog(ConstRef<Engine::Device> Device)
    {
        constexpr SDL_DialogFileFilter   kFilter[] = {
            { "Tileon Project", "tileon" }
        };
        constexpr SDL_DialogFileCallback kCallback = [](Ptr<void> Context, ConstPtr<ConstPtr<Char>> Files, SInt32)
        {
            if (Files && Files[0])
            {
                static_cast<Ptr<Bootstrap>>(Context)->OnDialogResult(Files[0]);
            }
        };
        SDL_ShowOpenFileDialog(kCallback, this, Device.GetHandle(), kFilter, 1, nullptr, false);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Bootstrap::OpenSaveDialog(ConstRef<Engine::Device> Device)
    {
        constexpr SDL_DialogFileFilter   kFilter[] = {
            { "Tileon Project", "tileon" }
        };
        constexpr SDL_DialogFileCallback kCallback = [](Ptr<void> Context, ConstPtr<ConstPtr<Char>> Files, SInt32)
        {
            if (Files && Files[0])
            {
                static_cast<Ptr<Bootstrap>>(Context)->OnDialogResult(Files[0]);
            }
        };
        SDL_ShowSaveFileDialog(kCallback, this, Device.GetHandle(), kFilter, 1, nullptr);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Bootstrap::OnDialogResult(ConstStr8 Path)
    {
        // Convert the file path to use backslashes, ensuring compatibility with windows file system conventions.
        Str8 Result(Path);
        std::ranges::replace(Result, '\\', '/');

        // Ensure the selected file has the correct project file extension, appending it if necessary.
        if (!Result.ends_with(Project::kExtension))
        {
            Result.append(Project::kExtension);
        }
        mProject.SetPath(Move(Result));

        switch (mState)
        {
        case State::Menu:
        {
            if (const Blob File = Filesystem::Load(mProject.GetPath()); File)
            {
                TOMLParser Parser(File.GetText());

                if (mProject.Load(Parser))
                {
                    mState = State::Done;
                }
            }
            break;
        }
        case State::Wizard:
        {
            TOMLParser Parser;
            mProject.Save(Parser);

            if (Scaffold(mProject.GetPath()))
            {
                Filesystem::Save(mProject.GetPath(), Parser.Dump());

                mState = State::Done;
            }
            else
            {
                LOG_WARNING("Bootstrap: Failed to prepare project directory at '{}'", mProject.GetPath());

                Filesystem::Delete(mProject.GetPath());
            }
            break;
        }
        default:
            break;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Bootstrap::Scaffold(ConstStr8 Path)
    {
        const Str8 Folder(mProject.GetPath().substr(0, mProject.GetPath().find_last_of("\\/") + 1));

        Bool Result = Filesystem::CopyAll(Filesystem::GetBase() + "\\Bootstrap\\", Folder);
        Result      = Result && Filesystem::Make(Folder + "Data");
        Result      = Result && Filesystem::Make(Folder + "Material");
        Result      = Result && Filesystem::Make(Folder + "World");
        return Result;
    }
}

