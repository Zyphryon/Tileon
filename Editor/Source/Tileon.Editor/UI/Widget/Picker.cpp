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

#include "Picker.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::UI
{

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Picker::Picker()
        : mOpen      { false },
          mMode      { Mode::Open },
          mSelection { -1 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Picker::Open(Mode Mode, Text Directory, Text Extension, AnyRef<OnResult> Callback)
    {
        mMode      = Mode;
        mDirectory = Directory;
        mExtension = Extension;
        mFilename.Clear();
        mSelection = -1;
        mCallback  = Move(Callback);
        mOpen      = true;

        Refresh();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Picker::Refresh()
    {
        mEntries.Clear();
        mSelection = -1;

        Filesystem::Enumerate(mDirectory, [this](ConstRef<Filesystem::Record> Record) -> Bool
        {
            const Bool IsDirectory = (Record.Type == Filesystem::Type::Directory);

            // Filter files by extension when one is specified; always show directories for navigation.
            if (IsDirectory || StrEndsWith(Record.Name, mExtension))
            {
                mEntries.Append(Record);
            }
            return true;
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Picker::Navigate(Text Directory)
    {
        mDirectory = Directory;
        Refresh();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Picker::Confirm()
    {
        if (!mFilename.IsEmpty())
        {
            mCallback(Filesystem::Path::Join(mDirectory, '/', mFilename));

            mOpen = false;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Picker::Draw(Ref<Composer> Composer)
    {
        if (!mOpen)
        {
            return;
        }

        Composer.SetNextWindowSizeConstraints(560.0f, 420.0f);

        if (!Composer.IsPopupOpen("##picker"))
        {
            Composer.OpenPopup("##picker");
        }

        if (Composer.BeginPopupModal("##picker", ImGuiWindowFlags_NoSavedSettings))
        {
            // Breadcrumb of the folder currently being browsed.
            Composer.Field(String<1024>::Print<"{0}  {1}">(ICON_FA_FOLDER_OPEN, mDirectory));
            Composer.Separator();

            // Directory / file listing.
            Composer.BeginChild("##picker_list", ImVec2(0, -Composer.GetFrameHeightWithSpacing() * 2.0f));
            {
                // ".." entry to navigate up, unless already at a presumed root.
                if (Composer.Selectable(ICON_FA_TURN_UP "  ..", false))
                {
                    Text Parent = mDirectory;

                    if (!mDirectory.IsEmpty() && mDirectory.GetBack() == '/')
                    {
                        Parent = mDirectory.Slice(0, mDirectory.GetSize() - 1);
                    }
                    Navigate(StrBeforeLast(Parent, '/'));
                }

                for (UInt32 Index = 0, Limit = mEntries.GetSize(); Index < Limit; ++Index)
                {
                    ConstRef<Filesystem::Record> Item = mEntries[Index];

                    const Bool IsDirectory = (Item.Type == Filesystem::Type::Directory);
                    const Bool WasSelected = (static_cast<SInt32>(Index) == mSelection);

                    Filesystem::Name Label = Filesystem::Name::Print<"{0}  {1}">(
                        IsDirectory ? ICON_FA_FOLDER : ICON_FA_FILE, Item.Name);

                    // Tint folders so they read apart from files at a glance.
                    if (IsDirectory)
                    {
                        Composer.PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.75f, 0.98f, 1.00f));
                    }

                    const Bool Clicked = Composer.Selectable(Label, WasSelected);

                    if (IsDirectory)
                    {
                        Composer.PopStyleColor();
                    }

                    if (Clicked)
                    {
                        if (IsDirectory)
                        {
                            Navigate(Filesystem::Path::Join(mDirectory, '/', Item.Name));
                            break;
                        }

                        mSelection = static_cast<SInt32>(Index);
                        mFilename  = Item.Name;
                    }
                }
            }
            Composer.EndChild();

            Composer.Separator();

            // Filename input — editable in Save mode, echoes the selection in Open mode.
            Composer.SetNextItemWidth(-1.0f);
            Composer.InputText("##picker_name", mFilename, [this](Text Value)
            {
                mFilename = Value;
            });

            const Bool CanConfirm   = !mFilename.IsEmpty();
            const Text ConfirmLabel = (mMode == Mode::Save)
                ? Text(ICON_FA_FLOPPY_DISK "  Save")
                : Text(ICON_FA_FOLDER_OPEN "  Open");

            Composer.PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.40f, 0.62f, 1.00f));
            Composer.PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.50f, 0.74f, 1.00f));
            Composer.PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.28f, 0.56f, 0.82f, 1.00f));
            const Bool Confirmed = Composer.DisabledButton(ConfirmLabel, !CanConfirm, 120.0f);
            Composer.PopStyleColor(3);

            if (Confirmed)
            {
                Confirm();
                Composer.CloseCurrentPopup();
            }

            Composer.SameLine();

            if (Composer.Button(ICON_FA_XMARK "  Cancel", 120.0f))
            {
                mOpen = false;
                Composer.CloseCurrentPopup();
            }

            Composer.EndPopup();
        }
    }
}