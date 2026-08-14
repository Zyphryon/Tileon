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

#include "Explorer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Explorer::Explorer()
        : mOpen      { false },
          mMode      { Mode::Open },
          mColumn    { Column::Name },
          mAscending { true },
          mEditing   { false },
          mCursor    { -1 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Explorer::Open(Mode Mode, Text Directory, Text Extension, AnyRef<OnResult> Callback)
    {
        mMode      = Mode;
        mExtension = Extension;
        mColumn    = Column::Name;
        mAscending = true;
        mEditing   = false;
        mCallback  = Move(Callback);
        mOpen      = true;

        mAddress.Clear();
        mFilename.Clear();
        mSearch.Clear();
        mHistory.Clear();
        mCursor = -1;

        Navigate(Directory);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Explorer::Refresh()
    {
        mEntries.Clear();

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

        Sort();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Explorer::Sort()
    {
        // Names collate without regard to case, so "Assets" and "assets" land next to each other.
        const auto Collate = [](Text Left, Text Right) -> SInt32
        {
            const UInt Length = (Left.GetSize() < Right.GetSize() ? Left.GetSize() : Right.GetSize());

            for (UInt Index = 0; Index < Length; ++Index)
            {
                const Char First  = StrLowercase(Left[Index]);
                const Char Second = StrLowercase(Right[Index]);

                if (First != Second)
                {
                    return First < Second ? -1 : 1;
                }
            }

            if (Left.GetSize() == Right.GetSize())
            {
                return 0;
            }
            return Left.GetSize() < Right.GetSize() ? -1 : 1;
        };

        const auto Compare = [this, &Collate](ConstRef<Filesystem::Record> Left, ConstRef<Filesystem::Record> Right)
        {
            const Bool LeftIsDirectory  = (Left.Type  == Filesystem::Type::Directory);
            const Bool RightIsDirectory = (Right.Type == Filesystem::Type::Directory);

            // Directories head the listing whichever way the column is ordered.
            if (LeftIsDirectory != RightIsDirectory)
            {
                return LeftIsDirectory;
            }

            SInt32 Order = 0;

            switch (mColumn)
            {
            case Column::Size:
                Order = (Left.Size == Right.Size) ? 0 : (Left.Size < Right.Size ? -1 : 1);
                break;
            case Column::Type:
                Order = Collate(StrAfterLast(Left.Name, '.'), StrAfterLast(Right.Name, '.'));
                break;
            default:
                break;
            }

            // Fall back to the name, so entries sharing a key keep a stable order of their own.
            if (Order == 0)
            {
                Order = Collate(Left.Name, Right.Name);
            }
            return mAscending ? Order < 0 : Order > 0;
        };

        mEntries.Sort(Compare);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Explorer::Navigate(Text Directory)
    {
        // The caller often hands us a view into mDirectory itself, so take a copy before anything reassigns it.
        const Filesystem::Path Target(Directory);

        // Branching off mid-trail discards whatever the user had moved back from.
        while (mHistory.GetSize() > static_cast<UInt>(mCursor + 1))
        {
            mHistory.RemoveLast();
        }

        mHistory.Append(Target);
        mCursor    = static_cast<SInt32>(mHistory.GetSize()) - 1;
        mDirectory = Target;

        Refresh();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Explorer::Travel(SInt32 Steps)
    {
        if (const SInt32 Target = mCursor + Steps; Target >= 0 && Target < static_cast<SInt32>(mHistory.GetSize()))
        {
            mCursor    = Target;
            mDirectory = mHistory[static_cast<UInt>(Target)];

            Refresh();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Explorer::Ascend()
    {
        // A trailing separator would otherwise resolve the parent to the directory itself.
        const Text Trimmed = (!mDirectory.IsEmpty() && mDirectory.GetBack() == '/')
            ? Text(mDirectory).Slice(0, mDirectory.GetSize() - 1)
            : Text(mDirectory);

        // Stop at the drive or filesystem root rather than navigating to nothing.
        if (const Text Parent = StrBeforeLast(Trimmed, '/'); !Parent.IsEmpty())
        {
            // A bare drive letter needs its separator back before it names a directory.
            Navigate(StrEndsWith(Parent, ":") ? Text(Filesystem::Path::Join(Parent, '/', Text::Empty())) : Parent);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Explorer::Locate(Text Address)
    {
        // Probing stops at the first entry, since only whether the directory opens matters here.
        const auto Probe = [](Text Path)
        {
            return Filesystem::Enumerate(Path, [](ConstRef<Filesystem::Record>)
            {
                return false;
            }) == Filesystem::Result::Success;
        };

        Filesystem::Path Target(Address);

        // Accept either separator style, and drop a trailing one so the parent resolves correctly.
        Target.Replace('\\', '/');

        while (Target.GetSize() > 1 && Target.GetBack() == '/')
        {
            Target.RemoveLast();
        }

        if (Target.IsEmpty())
        {
            return false;
        }

        if (Probe(Target))
        {
            Navigate(Target);
            return true;
        }

        // Not a directory, so read it as a full file path and land on the folder holding it.
        const Text Parent = StrBeforeLast(Target, '/');
        const Text Name   = StrAfterLast(Target, '/');

        if (!Parent.IsEmpty() && !Name.IsEmpty() && Probe(Parent))
        {
            const Filesystem::Name Selected(Name);

            Navigate(Parent);

            mFilename = Selected;
            return true;
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Explorer::Confirm()
    {
        if (mFilename.IsEmpty())
        {
            return false;
        }

        // Confirming a folder steps into it rather than returning it, as the system dialog does.
        for (ConstRef<Filesystem::Record> Item : mEntries)
        {
            if (Item.Type == Filesystem::Type::Directory && Text(Item.Name) == Text(mFilename))
            {
                const Filesystem::Path Target = Filesystem::Path::Join(mDirectory, '/', Item.Name);

                mFilename.Clear();
                Navigate(Target);

                return false;
            }
        }

        mCallback(Filesystem::Path::Join(mDirectory, '/', mFilename));

        mOpen = false;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Explorer::DrawToolbar()
    {
        constexpr Real32 kSearchWidth = 220.0f;

        const Bool CanGoBack    = (mCursor > 0);
        const Bool CanGoForward = (mCursor + 1 < static_cast<SInt32>(mHistory.GetSize()));

        if (Toolkit::Composer::DisabledButton(ICON_FA_ARROW_LEFT, !CanGoBack))
        {
            Travel(-1);
        }
        Toolkit::Composer::Tooltip("Back");

        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::DisabledButton(ICON_FA_ARROW_RIGHT, !CanGoForward))
        {
            Travel(1);
        }
        Toolkit::Composer::Tooltip("Forward");

        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::Button(ICON_FA_ARROW_UP))
        {
            Ascend();
        }
        Toolkit::Composer::Tooltip("Up one level");

        Toolkit::Composer::SameLine();

        const Real32 Spacing = Toolkit::Composer::GetStyle().ItemSpacing.x;
        const Real32 Width   = Toolkit::Composer::GetContentRegionAvail().x - kSearchWidth - Spacing;

        // Typing a location outright, the way Ctrl+L turns the address bar into a field.
        if (mEditing)
        {
            Toolkit::Composer::SetNextItemWidth(Width);

            if (!Toolkit::Composer::IsAnyItemActive())
            {
                Toolkit::Composer::SetKeyboardFocusHere();
            }

            Toolkit::Composer::InputText("##explorer_address", mAddress, [this](Text Value)
            {
                mAddress = Value;

                if (Locate(mAddress))
                {
                    mEditing = false;
                }
            }, ImGuiInputTextFlags_EnterReturnsTrue);

            // Clicking away or pressing Escape hands the trail back without moving anywhere.
            if (Toolkit::Composer::IsItemDeactivated())
            {
                mEditing = false;
            }
        }
        else
        {
            // The strip is one frame tall and never scrolls; a path too long for it sheds its leading crumbs.
            Toolkit::Composer::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            Toolkit::Composer::BeginChild("##explorer_trail", ImVec2(Width, Toolkit::Composer::GetFrameHeight()));
            Toolkit::Composer::PopStyleVar();
            {
                DrawTrail(Width);
            }
            Toolkit::Composer::EndChild();
        }

        Toolkit::Composer::SameLine();

        Toolkit::Composer::AlignTextToFramePadding();
        Toolkit::Composer::TextDisabled(ICON_FA_MAGNIFYING_GLASS);

        Toolkit::Composer::SameLine();

        Toolkit::Composer::SetNextItemWidth(-1.0f);
        Toolkit::Composer::InputText("##explorer_search", mSearch, [this](Text Value)
        {
            mSearch = Value;
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Explorer::DrawTrail(Real32 Width)
    {
        constexpr UInt kMaxSegments = 64;

        ConstRef<ImGuiStyle> Style = Toolkit::Composer::GetStyle();

        const Real32 Divider = Toolkit::Composer::CalcTextSize(ICON_FA_ANGLE_RIGHT).x + 4.0f;
        const Real32 Padding = Style.FramePadding.x * 2.0f;
        const Real32 Chevron = Toolkit::Composer::CalcTextSize(ICON_FA_ANGLE_LEFT).x + Padding;

        // Every boundary is recorded, so a segment starts one past the end of the one before it.
        Sequence<UInt, kMaxSegments> Bounds;

        for (UInt Cursor = 0, Limit = mDirectory.GetSize(); Cursor <= Limit && !Bounds.IsFull(); )
        {
            UInt End = Cursor;

            while (End < Limit && mDirectory[End] != '/')
            {
                ++End;
            }

            Bounds.Append(End);
            Cursor = End + 1;
        }

        // Walking back from the current folder keeps the crumbs that matter most when room runs short.
        UInt   First = 0;
        Real32 Total = 0.0f;

        for (UInt Slot = Bounds.GetSize(); Slot > 0; --Slot)
        {
            const UInt Index = Slot - 1;
            const UInt Start = (Index == 0) ? 0 : Bounds[Index - 1] + 1;

            if (Bounds[Index] <= Start)
            {
                continue;
            }

            const Text   Segment = Text(mDirectory).Slice(Start, Bounds[Index] - Start);
            const Real32 Step    = Toolkit::Composer::CalcTextSize(Segment).x + Padding + (Index > 0 ? Divider : 0.0f);

            if (Total + Step > Width - (Index > 0 ? Chevron : 0.0f))
            {
                First = Index + 1;
                break;
            }
            Total += Step;
        }

        Filesystem::Path Pending;
        Bool             IsFirst = true;

        // A leading chevron stands in for the crumbs that did not fit, and opens the field for typing.
        if (First > 0)
        {
            if (Toolkit::Composer::Button(ICON_FA_ANGLE_LEFT "##crumb_overflow"))
            {
                mAddress = mDirectory;
                mEditing = true;
            }
            IsFirst = false;
        }

        for (UInt Index = First, Limit = Bounds.GetSize(); Index < Limit; ++Index)
        {
            const UInt Start = (Index == 0) ? 0 : Bounds[Index - 1] + 1;

            if (Bounds[Index] <= Start)
            {
                continue;
            }

            const Text Segment = Text(mDirectory).Slice(Start, Bounds[Index] - Start);

            if (!IsFirst)
            {
                Toolkit::Composer::SameLine(0.0f, 2.0f);
                Toolkit::Composer::AlignTextToFramePadding();
                Toolkit::Composer::TextDisabled(ICON_FA_ANGLE_RIGHT);
                Toolkit::Composer::SameLine(0.0f, 2.0f);
            }

            // The offset disambiguates the button, since a path may repeat a folder name.
            if (Toolkit::Composer::Button(Filesystem::Path::Print<"{0}##crumb{1}">(Segment, Start)))
            {
                Pending = Text(mDirectory).Slice(0, Bounds[Index]);
            }
            IsFirst = false;
        }

        // Clicking the empty stretch past the trail opens the field, as the system dialog does.
        Toolkit::Composer::SameLine(0.0f, 2.0f);

        if (const Real32 Rest = Toolkit::Composer::GetContentRegionAvail().x; Rest > 1.0f)
        {
            if (Toolkit::Composer::InvisibleButton("##explorer_address_hit",
                ImVec2(Rest, Toolkit::Composer::GetFrameHeight())))
            {
                mAddress = mDirectory;
                mEditing = true;
            }
        }

        // Applied once the trail is drawn, since navigating rewrites the path being walked.
        if (!Pending.IsEmpty())
        {
            Navigate(Pending);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Explorer::DrawListing()
    {
        constexpr ImGuiTableFlags kTableFlags =
              ImGuiTableFlags_Sortable
            | ImGuiTableFlags_RowBg
            | ImGuiTableFlags_ScrollY
            | ImGuiTableFlags_Resizable
            | ImGuiTableFlags_BordersInnerV
            | ImGuiTableFlags_SizingStretchProp;

        constexpr ImGuiSelectableFlags kRowFlags =
            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick;

        // Sizes read the way the system dialog shows them.
        const auto Measure = [](UInt64 Bytes)
        {
            constexpr UInt64 kUnit = 1024;

            if (Bytes >= kUnit * kUnit * kUnit)
            {
                return String<32>::Print<"{0:.1f} GB">(static_cast<Real64>(Bytes) / (kUnit * kUnit * kUnit));
            }
            if (Bytes >= kUnit * kUnit)
            {
                return String<32>::Print<"{0:.1f} MB">(static_cast<Real64>(Bytes) / (kUnit * kUnit));
            }
            if (Bytes >= kUnit)
            {
                return String<32>::Print<"{0} KB">((Bytes + kUnit - 1) / kUnit);
            }
            return String<32>::Print<"{0} bytes">(Bytes);
        };

        // The type column spells the extension out in capitals, e.g. "TILEON File".
        const auto Describe = [](Text Name)
        {
            const Text Extension = StrAfterLast(Name, '.');

            if (Extension.IsEmpty())
            {
                return String<32>("File");
            }

            String<32> Capitals;

            for (UInt Index = 0, Limit = Extension.GetSize(); Index < Limit; ++Index)
            {
                Capitals.Append(StrUppercase(Extension[Index]));
            }
            return String<32>::Print<"{0} File">(Capitals);
        };

        // The search box narrows the listing in place, without re-reading the directory.
        const auto Matches = [this](Text Name)
        {
            if (mSearch.IsEmpty())
            {
                return true;
            }

            for (UInt Index = 0, Limit = Name.GetSize(); Index + mSearch.GetSize() <= Limit; ++Index)
            {
                UInt Cursor = 0;

                while (Cursor < mSearch.GetSize() && StrLowercase(Name[Index + Cursor]) == StrLowercase(mSearch[Cursor]))
                {
                    ++Cursor;
                }

                if (Cursor == mSearch.GetSize())
                {
                    return true;
                }
            }
            return false;
        };

        Bool             WasConfirmed = false;
        Bool             WasAscended  = false;
        Filesystem::Path Pending;

        // A scrolling table needs an exact height: bottom-aligning one collapses it while the popup is still
        // settling its own size, which hid the first rows on the frame the explorer opened.
        const Real32 Reserved  = Toolkit::Composer::GetFrameHeightWithSpacing() * 2.0f;
        const Real32 Remaining = Toolkit::Composer::GetContentRegionAvail().y - Reserved;
        const Real32 Height    = (Remaining > Reserved ? Remaining : Reserved);

        if (Toolkit::Composer::BeginTable("##explorer_list", 3, kTableFlags, ImVec2(0.0f, Height)))
        {
            Toolkit::Composer::TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort, 4.0f);
            Toolkit::Composer::TableSetupColumn("Size", ImGuiTableColumnFlags_None, 1.0f);
            Toolkit::Composer::TableSetupColumn("Type", ImGuiTableColumnFlags_None, 2.0f);
            Toolkit::Composer::TableSetupScrollFreeze(0, 1);
            Toolkit::Composer::TableHeadersRow();

            // Adopt whatever ordering the header row asks for.
            if (const Ptr<ImGuiTableSortSpecs> Specs = Toolkit::Composer::TableGetSortSpecs();
                Specs && Specs->SpecsDirty && Specs->SpecsCount > 0)
            {
                mColumn    = static_cast<Column>(Specs->Specs[0].ColumnIndex);
                mAscending = (Specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending);

                Specs->SpecsDirty = false;

                Sort();
            }

            // The parent entry, so the trail stays reachable without going back to the toolbar.
            Toolkit::Composer::TableNextRow();
            Toolkit::Composer::TableNextColumn();

            if (Toolkit::Composer::Selectable(ICON_FA_TURN_UP "  ..", false, kRowFlags)
                && Toolkit::Composer::IsMouseDoubleClicked())
            {
                WasAscended = true;
            }

            Toolkit::Composer::TableNextColumn();
            Toolkit::Composer::TableNextColumn();
            Toolkit::Composer::TextDisabled("File folder");

            for (UInt Index = 0, Limit = mEntries.GetSize(); Index < Limit; ++Index)
            {
                ConstRef<Filesystem::Record> Item = mEntries[Index];

                if (!Matches(Item.Name))
                {
                    continue;
                }

                const Bool IsDirectory = (Item.Type == Filesystem::Type::Directory);

                Toolkit::Composer::TableNextRow();
                Toolkit::Composer::TableNextColumn();

                const Filesystem::Name Label = Filesystem::Name::Print<"{0}  {1}">(
                    IsDirectory ? ICON_FA_FOLDER : ICON_FA_FILE, Item.Name);

                // Tint folders so they read apart from files at a glance.
                if (IsDirectory)
                {
                    Toolkit::Composer::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.75f, 0.98f, 1.00f));
                }

                const Bool Clicked = Toolkit::Composer::Selectable(Label, Text(Item.Name) == Text(mFilename), kRowFlags);

                if (IsDirectory)
                {
                    Toolkit::Composer::PopStyleColor();
                }

                if (Clicked)
                {
                    mFilename = Item.Name;

                    // A double click opens what was hit: folders step in, files confirm the selection.
                    if (Toolkit::Composer::IsMouseDoubleClicked())
                    {
                        if (IsDirectory)
                        {
                            Pending = Filesystem::Path::Join(mDirectory, '/', Item.Name);
                        }
                        else
                        {
                            WasConfirmed = true;
                        }
                    }
                }

                Toolkit::Composer::TableNextColumn();

                if (!IsDirectory)
                {
                    Toolkit::Composer::TextDisabled(Measure(Item.Size));
                }

                Toolkit::Composer::TableNextColumn();
                Toolkit::Composer::TextDisabled(IsDirectory ? String<32>("File folder") : Describe(Item.Name));
            }

            Toolkit::Composer::EndTable();
        }

        // Applied once the table is closed, since navigating replaces the very entries it was walking.
        if (WasAscended)
        {
            Ascend();
        }
        else if (!Pending.IsEmpty())
        {
            Navigate(Pending);
        }
        return WasConfirmed && Confirm();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Explorer::DrawFooter()
    {
        constexpr Real32 kFilterWidth = 170.0f;
        constexpr Real32 kButtonWidth = 110.0f;

        const Real32 Spacing = Toolkit::Composer::GetStyle().ItemSpacing.x;

        // Filename row: the label, the editable name, then the type filter.
        Toolkit::Composer::AlignTextToFramePadding();
        Toolkit::Composer::Field("File name:");

        Toolkit::Composer::SameLine();

        Toolkit::Composer::SetNextItemWidth(
            Toolkit::Composer::GetContentRegionAvail().x - kFilterWidth - Spacing);
        Toolkit::Composer::InputText("##explorer_name", mFilename, [this](Text Value)
        {
            mFilename = Value;
        });

        Toolkit::Composer::SameLine();

        // Only one filter is ever supplied, so the box reports it rather than offering a choice.
        const Text   Suffix = StrStartsWith(mExtension, ".") ? Text(mExtension).Slice(1) : Text(mExtension);
        const String<64> Filter = Suffix.IsEmpty()
            ? String<64>("All Files (*.*)")
            : String<64>::Print<"*.{0}">(Suffix);

        Toolkit::Composer::BeginDisabled();
        Toolkit::Composer::SetNextItemWidth(kFilterWidth);

        if (Toolkit::Composer::BeginCombo("##explorer_filter", Filter))
        {
            Toolkit::Composer::EndCombo();
        }
        Toolkit::Composer::EndDisabled();

        // The buttons sit at the right edge of the row beneath, as they do in the system dialog.
        Toolkit::Composer::SetCursorPosX(Toolkit::Composer::GetWindowWidth() - kButtonWidth * 2.0f - Spacing
            - Toolkit::Composer::GetStyle().WindowPadding.x);

        const Text ConfirmLabel = (mMode == Mode::Save)
            ? Text(ICON_FA_FLOPPY_DISK "  Save")
            : Text(ICON_FA_FOLDER_OPEN "  Open");

        Bool WasFinished = false;

        Toolkit::Composer::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f, 0.40f, 0.62f, 1.00f));
        Toolkit::Composer::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.50f, 0.74f, 1.00f));
        Toolkit::Composer::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.28f, 0.56f, 0.82f, 1.00f));
        const Bool Confirmed = Toolkit::Composer::DisabledButton(ConfirmLabel, mFilename.IsEmpty(), kButtonWidth);
        Toolkit::Composer::PopStyleColor(3);

        if (Confirmed)
        {
            WasFinished = Confirm();
        }

        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::Button(ICON_FA_XMARK "  Cancel", kButtonWidth))
        {
            mOpen       = false;
            WasFinished = true;
        }
        return WasFinished;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Explorer::Draw()
    {
        if (!mOpen)
        {
            return;
        }

        Toolkit::Composer::SetNextWindowSize(780.0f, 500.0f, ImGuiCond_Appearing);
        Toolkit::Composer::SetNextWindowSizeConstraints(560.0f, 420.0f);

        if (!Toolkit::Composer::IsPopupOpen("##explorer"))
        {
            Toolkit::Composer::OpenPopup("##explorer");
        }

        // The explorer lays itself out to the window, so it never scrolls as a whole; only the listing does.
        constexpr ImGuiWindowFlags kWindowFlags =
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        if (Toolkit::Composer::BeginPopupModal("##explorer", kWindowFlags))
        {
            // Captured before the body draws, since a field that swallows a key deactivates itself mid-frame.
            const Bool WasTyping  = Toolkit::Composer::IsAnyItemActive();
            const Bool WasLocating = mEditing;

            DrawToolbar();
            Toolkit::Composer::Separator();

            Bool WasFinished = DrawListing();

            if (DrawFooter())
            {
                WasFinished = true;
            }

            // Shortcuts, as in the system dialog: Ctrl+L or F4 opens the address bar for typing, Enter
            // confirms, Backspace steps up and Escape cancels. None of them fire while a field has the keys.
            if (Toolkit::Composer::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_L)
                || Toolkit::Composer::IsKeyPressed(ImGuiKey_F4))
            {
                mAddress = mDirectory;
                mEditing = true;
            }
            else if (!WasFinished)
            {
                // Enter commits from the filename field too, but never while the address bar owns it.
                if (!WasLocating && (Toolkit::Composer::IsKeyPressed(ImGuiKey_Enter)
                    || Toolkit::Composer::IsKeyPressed(ImGuiKey_KeypadEnter)))
                {
                    WasFinished = Confirm();
                }
                else if (!WasTyping && Toolkit::Composer::IsKeyPressed(ImGuiKey_Escape))
                {
                    mOpen       = false;
                    WasFinished = true;
                }
                else if (!WasTyping && Toolkit::Composer::IsKeyPressed(ImGuiKey_Backspace))
                {
                    Ascend();
                }
            }

            if (WasFinished)
            {
                Toolkit::Composer::CloseCurrentPopup();
            }

            Toolkit::Composer::EndPopup();
        }
        else if (!Toolkit::Composer::IsPopupOpen("##explorer"))
        {
            // The popup was dismissed from outside, so drop the explorer instead of reopening it next frame.
            mOpen = false;
        }
    }
}
