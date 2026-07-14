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
            Filesystem::Path Path(mDirectory);
            Path.Append(mFilename);

            mCallback(Path);

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

        Composer.SetNextWindowSizeConstraints(480.0f, 360.0f);

        if (!Composer.IsPopupOpen("##picker"))
        {
            Composer.OpenPopup("##picker");
        }

        if (Composer.BeginPopupModal("##picker", ImGuiWindowFlags_NoSavedSettings))
        {
            Composer.Field(mDirectory);
            Composer.Separator();

            // Directory / file listing.
            Composer.BeginChild("##picker_list", ImVec2(0, -Composer.GetFrameHeightWithSpacing() * 2.0f));
            {
                // ".." entry to navigate up, unless already at a presumed root.
                if (Composer.Selectable("..", false))
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

                    const Text Label = IsDirectory
                        ? String<160>::Print<"[D] {0}">(Item.Name)
                        : String<160>::Print<"{0}">(Item.Name);

                    const Bool WasSelected = (static_cast<SInt32>(Index) == mSelection);

                    if (Composer.Selectable(Label, WasSelected))
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

            // Filename input — editable in Save mode, read-only display in Open mode.
            Composer.SetNextItemWidth(-1.0f);
            Composer.InputText("##picker_name", mFilename, [this](Text Value)
            {
                mFilename = Value;
            });

            const Bool CanConfirm = !mFilename.IsEmpty();

            if (Composer.DisabledButton(mMode == Mode::Save ? "Save" : "Open", !CanConfirm, 100.0f))
            {
                Confirm();
                Composer.CloseCurrentPopup();
            }

            Composer.SameLine();

            if (Composer.Button("Cancel", 100.0f))
            {
                mOpen = false;
                Composer.CloseCurrentPopup();
            }

            Composer.EndPopup();
        }
    }
}