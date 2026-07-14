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

    Bootstrap::Result Bootstrap::Draw(Ref<UI::Composer> Composer)
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
                Result = DrawInMenu(Composer);
                break;
            case State::Wizard:
                Result = DrawInWizard(Composer);
                break;
            default:
                break;
            }

            Composer.End();
        }

        mPicker.Draw(Composer);

        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bootstrap::Result Bootstrap::DrawInMenu(Ref<UI::Composer> Composer)
    {
        constexpr Real32 kButtonW = 160.0f;
        constexpr Real32 kButtonH = 32.0f;
        const Real32     kOffsetX = (Composer.GetWindowWidth() - kButtonW) * 0.5f;

        Composer.SetCursorPosX(kOffsetX);
        if (Composer.Button("Open", kButtonW, kButtonH))
        {
            OpenFileDialog();
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

    Bootstrap::Result Bootstrap::DrawInWizard(Ref<UI::Composer> Composer)
    {
        constexpr Real32 kLabelW  = 90.0f;
        constexpr Real32 kButtonW = 80.0f;

        Composer.Label("Name");
        Composer.SameLine(kLabelW);
        Composer.SetNextItemWidth(-1.0f);
        Composer.InputText("##name", mProject.GetName(), [this](Text Value)
        {
            mProject.SetName(Value);
        });

        constexpr Array<UInt16, 6> kDensityOptions = { 8, 16, 32, 64, 128, 256 };

        Composer.Label("Density");
        Composer.SameLine(kLabelW);
        Composer.SetNextItemWidth(-1.0f);
        if (Composer.BeginCombo("##density", String<32>::Print<"{0}">(mProject.GetDensity())))
        {
            for (const UInt16 Option : kDensityOptions)
            {
                const Bool Selected = (mProject.GetDensity() == Option);

                if (Composer.Selectable(String<32>::Print<"{0}">(Option), Selected))
                {
                    mProject.SetDensity(Option);
                }
            }
            Composer.EndCombo();
        }

        Composer.Label("Description");
        Composer.SameLine(kLabelW);
        Composer.SetNextItemWidth(-1.0f);
        Composer.InputText("##description", mProject.GetDescription(), [this](Text Value)
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
            OpenSaveDialog();
        }

        return Result::None;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Bootstrap::OpenFileDialog()
    {
        mPicker.Open(UI::Picker::Mode::Open, Filesystem::GetRootFolder(), Project::kFileExtension,
            [this](Text Path)
            {
                OnDialogResult(Path);
            });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Bootstrap::OpenSaveDialog()
    {
        mPicker.Open(UI::Picker::Mode::Save, Filesystem::GetRootFolder(), Project::kFileExtension,
            [this](Text Path)
            {
                OnDialogResult(Path);
            });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Bootstrap::OnDialogResult(Text Path)
    {
        // Convert the file path to use backslashes, ensuring compatibility with windows file system conventions.
        Str Result(Path);

        // Ensure the selected file has the correct project file extension, appending it if necessary.
        if (!StrEndsWith(Result, Project::kFileExtension))
        {
            Result.Append('.');
            Result.Append(Project::kFileExtension);
        }
        mProject.SetPath(Move(Result));

        switch (mState)
        {
        case State::Menu:
        {
            Blob File;

            if (Filesystem::Read(mProject.GetPath(), File) == Filesystem::Result::Success)
            {
                JsonValue Parser = JsonDocument::Parse(Text(File.GetData<Char>(), File.GetSize()));

                if (mProject.Load(Parser))
                {
                    mState = State::Done;
                }
            }
            break;
        }
        case State::Wizard:
        {
            JsonValue Parser;
            Parser.SetObject();
            mProject.Save(Parser);

            if (Scaffold(StrBeforeLast(mProject.GetPath(), '/')))
            {
                const Str Data = JsonDocument::Dump(Parser);
                Filesystem::Write(mProject.GetPath(),
                    ConstSpan(reinterpret_cast<ConstPtr<Byte>>(Data.GetData()), Data.GetSize()));

                mState = State::Done;
            }
            else
            {
                LOG_W("Bootstrap: Failed to prepare project directory at '{}'", mProject.GetPath());

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

    Bool Bootstrap::Scaffold(Text Path)
    {
        Filesystem::Path Source = Filesystem::GetRootFolder();
        Source.Append("Bootstrap/");

        if (Filesystem::CopyAll(Source, Path) == Filesystem::Result::Success)
        {
            Filesystem::Make(Path + Filesystem::Path("Data"));
            Filesystem::Make(Path + Filesystem::Path("Material"));
            Filesystem::Make(Path + Filesystem::Path("World"));
            return true;
        }
        return false;
    }
}

