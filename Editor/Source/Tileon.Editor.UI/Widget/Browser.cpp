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

#include "Browser.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Browser::Browser(Ref<Content::Service> Service, Mode Mode)
        : mService { Service },
          mMode    { Mode },
          mOpen    { false }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Browser::Open(Text Filter)
    {
        mFilter = Filter;

        if (mMode == Mode::Popup)
        {
            mOpen = true;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Browser::Draw(Ref<Composer> Composer)
    {
        // Draw the browser as a modal popup if in popup mode, otherwise draw it inline.
        Bool WasFinished = false;

        if (mMode == Mode::Popup)
        {
            if (mOpen)
            {
                Composer.OpenPopup("##browser_modal");
                mOpen = false;
            }

            Composer.SetNextWindowSize(800.0f, 520.0f, ImGuiCond_Always);
            if (Composer.BeginPopupModal("##browser_modal", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
            {
                WasFinished = DrawPopup(Composer);

                Composer.EndPopup();
            }
        }
        else
        {
            DrawBody(Composer);
        }
        return WasFinished;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Browser::DrawPopup(Ref<UI::Composer> Composer)
    {
        // Draw the main content area of the popup.
        const Real32 FooterHeight = Composer.GetFrameHeightWithSpacing() + 6.0f;
        Composer.BeginChild("##browser_modal_content", ImVec2(0.0f, -FooterHeight), ImGuiChildFlags_Borders);
        DrawBody(Composer);
        Composer.EndChild();

        Composer.Separator();

        // Draw the action buttons at the bottom of the popup.
        Bool WasFinished = false;

        if (Composer.DisabledButton("Open", mSelection.IsEmpty()))
        {
            WasFinished = true;

            Composer.CloseCurrentPopup();
        }

        Composer.SameLine();

        if (Composer.Button("Cancel"))
        {
            Reset();

            WasFinished = true;

            Composer.CloseCurrentPopup();
        }

        // Display the currently selected item, if any.
        if (!mSelection.IsEmpty())
        {
            Composer.SameLine();

            Composer.TextDisabled(mSelection);
        }
        return WasFinished;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Browser::DrawBody(Ref<Composer> Composer)
    {
        Composer.BeginChild("##browser_sidebar", ImVec2(180.0f, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
        Composer.SetNextItemOpen(true, ImGuiCond_Once);
        if (Composer.TreeNode("Content", ImGuiTreeNodeFlags_SpanFullWidth))
        {
            DrawSidebarTree(Composer, "Resources://");  // TODO: Specify Schema?
            Composer.TreePop();
        }
        Composer.EndChild();

        Composer.SameLine();

        Composer.BeginChild("##browser_content", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
        DrawContent(Composer);
        Composer.EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Browser::Reset()
    {
        mPath      = Content::Uri();
        mSelection = "";

        // Drop the cache so the next time the browser is shown it re-enumerates and reflects on-disk changes.
        mEntries.Clear();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Browser::DrawSidebarTree(Ref<Composer> Composer, ConstRef<Content::Uri> Parent)
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

            const Bool Open = Composer.TreeNode(Entry.Name, Flags);

            if (Composer.IsItemClicked() && !Composer.IsItemToggledOpen())
            {
                mPath      = ChildUri;
                mSelection = "";
                mGallery.SetSelection(0);
            }

            if (Open)
            {
                DrawSidebarTree(Composer, ChildUri);

                Composer.TreePop();
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Browser::DrawContent(Ref<Composer> Composer)
    {
        mGallery.DrawToolbar(Composer);
        Composer.Separator();

        mGallery.Begin(Composer);

        for (ConstRef<Filesystem::Record> Entry : GetEntries(mPath))
        {
            if (Entry.Type != Filesystem::Type::File)
            {
                continue;
            }

            // Apply the current filter, skipping it if it doesn't match.
            if (!mFilter.IsEmpty() && !StrEndsWith(Entry.Name, mFilter))
            {
                continue;
            }

            const UInt32 ID = Hash(Entry.Name);

            if (mGallery.DrawItem(Composer, ID, Entry.Name))
            {
                mSelection.Format<"{0}{1}">(mPath.GetUrl(), Entry.Name);
            }
        }

        mGallery.End(Composer);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Browser::Entries Browser::GetEntries(ConstRef<Content::Uri> Uri)
    {
        const UInt64 Key = Hash(Uri.GetPath());

        // Decide whether a (re)enumeration is due without holding a reference across the request.
        Ref<Directory> Slot = mEntries.FindOrInsert(Key);

        if (const Real64 Now = ImGui::GetTime(); !Slot.Pending && Now >= Slot.Refresh)
        {
            Slot.Pending = true;
            Slot.Refresh = Now + kInterval;

            // The enumeration runs asynchronously and its completion is delivered on the main thread, so the cache can
            // be updated here without any synchronization.
            mService.Enumerate(Uri, [this, Key](Filesystem::Result Result, Sequence<Filesystem::Record> Records)
            {
                Ref<Directory> Entry = mEntries.FindOrInsert(Key);
                Entry.Records = Move(Records);
                Entry.Pending = false;
            });
        }

        // Re-fetch after the request, since a synchronous completion may have already mutated the table.
        return mEntries.FindOrInsert(Key).Records;
    }
}

