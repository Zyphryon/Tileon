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

namespace Tileon::Editor::Panel
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool AccentButton(Ref<UI::Composer> Composer, Text Label, Real32 Width, Real32 Height = 0.0f)
    {
        Composer.PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.40f, 0.62f, 1.00f));
        Composer.PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.50f, 0.74f, 1.00f));
        Composer.PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.28f, 0.56f, 0.82f, 1.00f));
        const Bool Pressed = Composer.Button(Label, Width, Height);
        Composer.PopStyleColor(3);
        return Pressed;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bootstrap::Bootstrap()
        : mState { State::Menu }
    {
        LoadRecent();
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
            ImGui::SetNextWindowSize(ImVec2(300, 380));
            break;
        case State::Wizard:
            ImGui::SetNextWindowSize(ImVec2(460, 320));
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
        const Real32 Width = Composer.GetContentRegionAvail().x;

        ImGui::SetWindowFontScale(2.4f);
        Composer.CenterCursor("Welcome");
        Composer.TextColored(ImVec4(0.46f, 0.72f, 0.98f, 1.00f), "Welcome");
        ImGui::SetWindowFontScale(1.0f);

        Composer.Spacing();
        Composer.Separator();
        Composer.Spacing();

        constexpr Real32 kHeight = 38.0f;

        if (AccentButton(Composer, ICON_FA_FOLDER_PLUS "   Create New", Width, kHeight))
        {
            mState = State::Wizard;
        }

        Composer.Spacing();

        if (Composer.Button(ICON_FA_FOLDER_OPEN "   Open Project", Width, kHeight))
        {
            OpenFileDialog();
        }

        // Recent projects, most-recently opened first.
        if (!mRecent.IsEmpty())
        {
            Composer.Spacing();
            Composer.Header("Recent");
            Composer.Spacing();

            SInt32 Clicked = -1;

            for (UInt32 Index = 0; Index < mRecent.GetSize(); ++Index)
            {
                ConstRef<Str> Path  = mRecent[Index];
                const Text    Label = StrAfterLast(Path, '/');

                if (Composer.Selectable(String<288>::Print<"{0}##recent_{1}">(Label, Index), false))
                {
                    Clicked = static_cast<SInt32>(Index);
                }
                Composer.Tooltip(Path);
            }

            if (Clicked >= 0)
            {
                const Str Path = mRecent[Clicked];
                OpenRecent(Path);
            }
        }

        Composer.SetCursorPosY(Composer.GetWindowBottom(2));
        Composer.Separator();
        Composer.Spacing();

#if !defined(ZY_PLATFORM_WEB)
        if (Composer.Button(ICON_FA_RIGHT_FROM_BRACKET "   Exit", Width, kHeight))
        {
            return Result::Exit;
        }
#endif

        return Result::None;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bootstrap::Result Bootstrap::DrawInWizard(Ref<UI::Composer> Composer)
    {
        constexpr Real32 kLabelW = 110.0f;

        ImGui::SetWindowFontScale(1.5f);
        Composer.TextColored(ImVec4(0.46f, 0.72f, 0.98f, 1.00f), ICON_FA_CUBES "  New Project");
        ImGui::SetWindowFontScale(1.0f);

        Composer.Spacing();
        Composer.Separator();
        Composer.Spacing();

        Composer.Label(ICON_FA_PEN_TO_SQUARE "  Name");
        Composer.SameLine(kLabelW);
        Composer.SetNextItemWidth(-1.0f);
        Composer.InputText("##name", mProject.GetName(), [this](Text Value)
        {
            mProject.SetName(Value);
        });

        Composer.Spacing();

        constexpr Array<UInt16, 6> kDensityOptions = { 8, 16, 32, 64, 128, 256 };

        Composer.Label(ICON_FA_LAYER_GROUP "  Density");
        Composer.SameLine(kLabelW);
        Composer.SetNextItemWidth(-1.0f);
        if (Composer.BeginCombo("##density", String<32>::Print<"{0} px">(mProject.GetDensity())))
        {
            for (const UInt16 Option : kDensityOptions)
            {
                const Bool Selected = (mProject.GetDensity() == Option);

                if (Composer.Selectable(String<32>::Print<"{0} px">(Option), Selected))
                {
                    mProject.SetDensity(Option);
                }
            }
            Composer.EndCombo();
        }

        Composer.Spacing();

        Composer.Label(ICON_FA_ALIGN_LEFT "  Description");
        Composer.SameLine(kLabelW);
        Composer.SetNextItemWidth(-1.0f);
        Composer.InputText("##description", mProject.GetDescription(), [this](Text Value)
        {
            mProject.SetDescription(Value);
        });

        Composer.SetCursorPosY(Composer.GetWindowBottom(2));
        Composer.Separator();
        Composer.Spacing();

        constexpr Real32 kButtonH = 34.0f;
        const Real32     kButtonW = (Composer.GetContentRegionAvail().x - Composer.GetStyle().ItemSpacing.x) * 0.5f;

        if (Composer.Button(ICON_FA_CHEVRON_LEFT "  Back", kButtonW, kButtonH))
        {
            mState = State::Menu;
        }

        Composer.SameLine();

        if (AccentButton(Composer, ICON_FA_CHECK "  Create", kButtonW, kButtonH))
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
                    PushRecent(mProject.GetPath());

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

                PushRecent(mProject.GetPath());

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
            Filesystem::Make(Path + Filesystem::Path("/Data"));
            Filesystem::Make(Path + Filesystem::Path("/Font"));
            Filesystem::Make(Path + Filesystem::Path("/Material"));
            Filesystem::Make(Path + Filesystem::Path("/World"));
            return true;
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Bootstrap::OpenRecent(Text Path)
    {
        mProject.SetPath(Str(Path));

        Blob File;

        if (Filesystem::Read(mProject.GetPath(), File) == Filesystem::Result::Success)
        {
            JsonValue Parser = JsonDocument::Parse(Text(File.GetData<Char>(), File.GetSize()));

            if (mProject.Load(Parser))
            {
                PushRecent(mProject.GetPath());

                mState = State::Done;
                return;
            }
        }

        // The file is gone or unreadable, so drop it from the list to stop it reappearing.
        mRecent.RemoveIf([&](ConstRef<Str> Existing)
        {
            return Existing == Path;
        });
        SaveRecent();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Bootstrap::LoadRecent()
    {
        mRecent.Clear();

        Blob File;

        if (Filesystem::Read(Filesystem::GetDataFolder("Tileon", "Editor") + "Recent.json", File) != Filesystem::Result::Success)
        {
            return;
        }

        JsonValue Document = JsonDocument::Parse(Text(File.GetData<Char>(), File.GetSize()));

        if (!Document.IsObject())
        {
            return;
        }

        const JsonObject Root(Document);
        const JsonArray  Items = Root.GetArray("Recent");

        if (Items.IsNull())
        {
            return;
        }

        for (UInt Index = 0; Index < Items.GetSize() && mRecent.GetSize() < kMaxRecent; ++Index)
        {
            if (const Text Path = Items.GetString(Index); !Path.IsEmpty())
            {
                mRecent.Append(Str(Path));
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Bootstrap::SaveRecent()
    {
        JsonValue Document;
        Document.SetObject();

        JsonObject Root(Document);
        JsonArray  Items = Root.SetArray("Recent");

        for (ConstRef<Str> Path : mRecent)
        {
            Items.AddString(Path);
        }

        const Str Data = JsonDocument::Dump(Document);
        Filesystem::Write(Filesystem::GetDataFolder("Tileon", "Editor") + "Recent.json",
            ConstSpan(reinterpret_cast<ConstPtr<Byte>>(Data.GetData()), Data.GetSize()));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Bootstrap::PushRecent(Text Path)
    {
        // Move the path to the front (most recent), de-duplicating any earlier appearance.
        mRecent.RemoveIf([&](ConstRef<Str> Existing)
        {
            return Existing == Path;
        });
        mRecent.Insert(0, Str(Path));

        // Drop the oldest entries once the cap is exceeded.
        while (mRecent.GetSize() > kMaxRecent)
        {
            mRecent.RemoveLast();
        }
        SaveRecent();
    }
}

