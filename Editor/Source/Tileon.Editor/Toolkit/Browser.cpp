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

#include "Tileon.Editor/Toolkit/Browser.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Toolkit
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Browser::Browser(Ref<Content::Service> Service)
        : mService { Service },
          mOpen    { false },
          mRequest { 0 },
          mResult  { 0 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Browser::Open(UInt64 Key, Text Filter)
    {
        // A second request supersedes the first, since only one browser exists to serve them.
        mRequest = Key;
        mResult  = 0;
        mAnswer  = "";
        mOpen    = true;

        // A field may take more than one kind of file, so the filter is a list of suffixes separated by spaces.
        mFilters.Clear();

        for (UInt Cursor = 0; Cursor < Filter.GetSize(); )
        {
            const SInt Break  = StrFind(Filter.Slice(Cursor), ' ');
            const UInt Length = (Break < 0 ? Filter.GetSize() - Cursor : static_cast<UInt>(Break));

            if (Length > 0)
            {
                mFilters.Append(Filter.Slice(Cursor, Length));
            }
            Cursor += Length + 1;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Browser::Accepts(Text Name) const
    {
        // A field that named no kind at all takes whatever the folder holds.
        if (mFilters.IsEmpty())
        {
            return true;
        }

        for (ConstRef<Str> Suffix : mFilters)
        {
            if (StrEndsWith(Name, Suffix))
            {
                return true;
            }
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Browser::Draw()
    {
        // Fields draw before this point, so an answer still sitting here was offered for a full frame and never
        // taken. Dropping it keeps a stale one from landing on whatever the user inspects next.
        mResult = 0;
        mAnswer = "";

        Bool WasFinished = false;

        Composer::PushID(this);

        if (mOpen)
        {
            Composer::OpenPopup("##browser_modal");
            mOpen = false;
        }

        Composer::SetNextWindowSize(800.0f, 520.0f, ImGuiCond_Always);
        if (Composer::BeginPopupModal("##browser_modal", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
        {
            WasFinished = DrawPopup();

            Composer::EndPopup();
        }

        Composer::PopID();

        if (!WasFinished)
        {
            return;
        }

        // Cancelling resets the browser, leaving the selection empty and the request simply dropped.
        if (!mSelection.IsEmpty())
        {
            mResult = mRequest;
            mAnswer = mSelection;
        }

        mRequest = 0;

        // Clear the confirmed selection too, so reopening starts fresh instead of showing the previous answer.
        Reset();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Browser::Consume(UInt64 Key, Ref<Str> Selection)
    {
        if (Key == 0 || mResult != Key)
        {
            return false;
        }

        Selection = mAnswer;

        mResult = 0;
        mAnswer = "";
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Browser::DrawPopup()
    {
        // Draw the main content area of the popup.
        const Real32 FooterHeight = Composer::GetFrameHeightWithSpacing() + 6.0f;
        Composer::BeginChild("##browser_modal_content", ImVec2(0.0f, -FooterHeight), ImGuiChildFlags_Borders);
        DrawBody();
        Composer::EndChild();

        Composer::Separator();

        // Draw the action buttons at the bottom of the popup.
        Bool WasFinished = false;

        if (Composer::DisabledButton("Open", mSelection.IsEmpty()))
        {
            WasFinished = true;

            Composer::CloseCurrentPopup();
        }

        Composer::SameLine();

        if (Composer::Button("Cancel"))
        {
            Reset();

            WasFinished = true;

            Composer::CloseCurrentPopup();
        }

        // Display the currently selected item, if any.
        if (!mSelection.IsEmpty())
        {
            Composer::SameLine();

            Composer::TextDisabled(mSelection);
        }
        return WasFinished;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Browser::DrawBody()
    {
        Composer::BeginChild("##browser_sidebar", ImVec2(180.0f, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
        Composer::SetNextItemOpen(true, ImGuiCond_Once);
        if (Composer::TreeNode("Content", ImGuiTreeNodeFlags_SpanFullWidth))
        {
            DrawSidebarTree("Resources://");  // TODO: Specify Schema?

            Composer::TreePop();
        }
        Composer::EndChild();

        Composer::SameLine();

        Composer::BeginChild("##browser_content", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
        DrawContent();
        Composer::EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Browser::Reset()
    {
        mPath      = Content::Uri();
        mSelection = "";

        // Drop the cache so the next time the browser is shown it re-enumerates and reflects on-disk changes.
        {
            Guard Lock(mMutex);

            mEntries.Clear();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Browser::DrawSidebarTree(ConstRef<Content::Uri> Parent)
    {
        for (ConstRef<Filesystem::Record> Entry : GetEntries(Parent))
        {
            if (Entry.Type != Filesystem::Type::Directory)
            {
                continue;
            }

            const Content::Uri ChildUri(Str::Print<"{0}{1}/">(Parent.GetUrl(), Entry.Name));

            // Check if the child directory is a leaf (i.e. it has no subdirectories).
            Bool IsLeaf = true;

            for (ConstRef<Filesystem::Record> Sub : GetEntries(ChildUri))
            {
                if (Sub.Type == Filesystem::Type::Directory)
                {
                    IsLeaf = false;
                    break;
                }
            }

            ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow;

            if (IsLeaf)
            {
                Flags |= ImGuiTreeNodeFlags_Leaf;
            }

            if (mPath.GetPath() == ChildUri.GetPath())
            {
                Flags |= ImGuiTreeNodeFlags_Selected;
            }

            const Bool Open = Composer::TreeNode(Entry.Name, Flags);

            if (Composer::IsItemClicked() && !Composer::IsItemToggledOpen())
            {
                mPath      = ChildUri;
                mSelection = "";
                mItems.SetSelection(0);
            }

            if (Open)
            {
                DrawSidebarTree(ChildUri);

                Composer::TreePop();
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Browser::DrawContent()
    {
        mItems.DrawToolbar();
        Composer::Separator();

        mItems.Begin();

        for (ConstRef<Filesystem::Record> Entry : GetEntries(mPath))
        {
            if (Entry.Type != Filesystem::Type::File)
            {
                continue;
            }

            // Apply the current filter, skipping it if it doesn't match.
            if (!Accepts(Entry.Name))
            {
                continue;
            }

            const UInt32 ID = Hash(Entry.Name);

            if (mItems.DrawItem(ID, Entry.Name))
            {
                mSelection.Format<"{0}{1}">(mPath.GetUrl(), Entry.Name);
            }
        }

        mItems.End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Browser::Entries Browser::GetEntries(ConstRef<Content::Uri> Uri)
    {
        const UInt64 Key = Hash(Uri.GetPath());

        // Decide whether a (re)enumeration is due without holding a reference (or the lock) across the request.
        Bool WasRequested = false;

        {
            Guard Lock(mMutex);

            Ref<Directory> Slot = mEntries.FindOrInsert(Key);

            if (const Real64 Now = Composer::GetTime(); !Slot.Pending && Now >= Slot.Refresh)
            {
                Slot.Pending = true;
                Slot.Refresh = Now + kInterval;

                WasRequested = true;
            }
        }

        // A synchronous mount completes inline on this thread, so the request is issued unlocked to avoid deadlocking
        // against the callback below, which otherwise runs on an I/O worker.
        if (WasRequested)
        {
            mService.Enumerate(Uri, [this, Key](Filesystem::Result Result, Sequence<Filesystem::Record> Records)
            {
                Guard Lock(mMutex);

                Ref<Directory> Entry = mEntries.FindOrInsert(Key);
                Entry.Records = Move(Records);
                Entry.Pending = false;
            });
        }

        // Copy under the lock, since the worker may replace the records while the caller iterates them.
        Guard Lock(mMutex);

        return mEntries.FindOrInsert(Key).Records;
    }
}