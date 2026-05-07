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
          mOpen    { false },
          mTime    { 0.0 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Browser::Open(ConstStr8 Filter)
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
        // Invalidate the directory cache periodically so changes on disk are reflected.
        if (ImGui::GetTime() >= mTime)
        {
            mEntries.clear();
            mTime = ImGui::GetTime() + kInterval;
        }

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

        if (Composer.DisabledButton("Open", mSelection.empty()))
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
        if (!mSelection.empty())
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
        mTime      = 0.0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Browser::DrawSidebarTree(Ref<Composer> Composer, ConstRef<Content::Uri> Parent)
    {
        for (ConstRef<Content::Mount::Item> Entry : GetEntries(Parent))
        {
            if (Entry.Type != Content::Mount::Entry::Directory)
            {
                continue;
            }

            const Content::Uri ChildUri(Format("{}{}/", Parent.GetUrl(), Entry.Name));

            // Check if the child directory is a leaf (i.e. it has no subdirectories).
            Bool IsLeaf = true;

            for (ConstRef<Content::Mount::Item> Sub : GetEntries(ChildUri))
            {
                if (Sub.Type == Content::Mount::Entry::Directory)
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

        for (ConstRef<Content::Mount::Item> Entry : GetEntries(mPath))
        {
            if (Entry.Type != Content::Mount::Entry::File)
            {
                continue;
            }

            // Apply the current filter, skipping it if it doesn't match.
            if (!mFilter.empty() && !Entry.Name.ends_with(mFilter))
            {
                continue;
            }

            const UInt32 ID = Hash(Entry.Name);

            if (mGallery.DrawItem(Composer, ID, Entry.Name))
            {
                mSelection = Format("{}{}", mPath.GetUrl(), Entry.Name);
            }
        }

        mGallery.End(Composer);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstRef<Browser::Entries> Browser::GetEntries(ConstRef<Content::Uri> Uri)
    {
        const UInt64 Key = Hash(Uri);

        auto Iterator = mEntries.find(Key);

        if (Iterator == mEntries.end())
        {
            Iterator = mEntries.emplace(Key, mService.Enumerate(Uri)).first;
        }
        return Iterator->second;
    }
}

