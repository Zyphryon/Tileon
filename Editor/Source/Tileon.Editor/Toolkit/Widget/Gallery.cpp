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

#include "Gallery.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Toolkit
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Gallery::Gallery()
        : mActive     { false },
          mMode       { Mode::Grid },
          mSize       { 64.0f },
          mSelection  { 0 },
          mActivated  { -1 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Gallery::DrawToolbar()
    {
        const Real32 Spacing = Toolkit::Composer::GetStyle().ItemSpacing.x;
        const Real32 Padding = Toolkit::Composer::GetStyle().FramePadding.x;

        // Pre-compute widths of the fixed controls.
        const Real32 ListBtnW  = Toolkit::Composer::CalcTextSize(ICON_FA_LIST).x              + Padding * 2.0f;
        const Real32 GridBtnW  = Toolkit::Composer::CalcTextSize(ICON_FA_TABLE_CELLS_LARGE).x + Padding * 2.0f;
        const Real32 SliderW   = (mMode == Mode::Grid) ? 80.0f : 0.0f;
        const Real32 SliderGap = (mMode == Mode::Grid) ? (Spacing + SliderW) : 0.0f;

        // Filter input stretches to fill remaining space.
        const Real32 FilterW = Toolkit::Composer::GetContentRegionAvail().x - ListBtnW - GridBtnW - SliderGap - Spacing * 2.0f;

        Toolkit::Composer::SetNextItemWidth(FilterW > 0.0f ? FilterW : 1.0f);
        Toolkit::Composer::InputText("##gallery_filter", mFilter, [this](Text Value)
        {
            mFilter = Value;
        });

        Toolkit::Composer::SameLine();

        // List mode toggle button.
        const Bool ListActive = (mMode == Mode::List);
        if (ListActive)
        {
            Toolkit::Composer::PushStyleColor(ImGuiCol_Button,        Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
            Toolkit::Composer::PushStyleColor(ImGuiCol_ButtonHovered, Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        if (Toolkit::Composer::Button(ICON_FA_LIST))
        {
            mMode = Mode::List;
        }
        if (ListActive)
        {
            Toolkit::Composer::PopStyleColor(2);
        }

        Toolkit::Composer::SameLine();

        // Grid mode toggle button.
        const Bool GridActive = (mMode == Mode::Grid);
        if (GridActive)
        {
            Toolkit::Composer::PushStyleColor(ImGuiCol_Button,        Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
            Toolkit::Composer::PushStyleColor(ImGuiCol_ButtonHovered, Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        if (Toolkit::Composer::Button(ICON_FA_TABLE_CELLS_LARGE))
        {
            mMode = Mode::Grid;
        }
        if (GridActive)
        {
            Toolkit::Composer::PopStyleColor(2);
        }

        // Cell-size slider, visible in grid mode only.
        if (mMode == Mode::Grid)
        {
            Toolkit::Composer::SameLine();
            Toolkit::Composer::SetNextItemWidth(SliderW);
            Toolkit::Composer::SliderFloat("##gallery_cellsize", mSize, kThumbnailMinSize, kThumbnailMaxSize, "%.0f");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Gallery::Begin()
    {
        // Right-click activation is a per-frame, one-shot signal, so clear it before the items are drawn.
        mActivated = -1;

        if (mMode == Mode::Grid)
        {
            const ImVec2 Available = Toolkit::Composer::GetContentRegionAvail();
            const Real32 Spacing   = Toolkit::Composer::GetStyle().ItemSpacing.x;
            const SInt32 Columns   = Max(1, static_cast<SInt32>((Available.x + Spacing) / (mSize + Spacing)));

            mActive = Toolkit::Composer::BeginTable("##gallery", Columns);

            if (mActive)
            {
                for (SInt32 I = 0; I < Columns; ++I)
                {
                    Toolkit::Composer::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, mSize);
                }
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Gallery::DrawItem(UInt32 ID, Text Name, ImTextureID Thumbnail, Rect Crop, IntColor8 Tint)
    {
        // Filter the item based on the provided name and the current filter string.
        if (!Filter(Name))
        {
            return false;
        }

        const Bool WasSelected = (mSelection == ID);

        // In list mode, render a selectable item with the name as the label.
        if (mMode == Mode::List)
        {
            const Text Label = String<128>::Print<"{0}##{1}">(Name, ID);

            const Bool Chosen = Toolkit::Composer::Selectable(Label, WasSelected);

            // A right-click both selects the item and flags it for opening in its dedicated editor.
            if (Toolkit::Composer::IsItemClicked(ImGuiMouseButton_Right))
            {
                mSelection = ID;
                mActivated = ID;
            }

            if (Chosen)
            {
                mSelection = ID;
                return true;
            }
            return false;
        }

        // In grid mode, render a thumbnail cell.
        if (!mActive)
        {
            return false;
        }

        Toolkit::Composer::TableNextColumn();

        // A cell scrolled out of view still costs an id, a hit test and a textured draw command, and a
        // thumbnail of its own defeats batching, so the row is only reserved and nothing is submitted.
        if (!Toolkit::Composer::IsRectVisible(ImVec2(mSize, mSize)))
        {
            Toolkit::Composer::Dummy(ImVec2(mSize, mSize));

            return false;
        }

        const ImVec2 Origin    = Toolkit::Composer::GetCursorScreenPos();
        const ImVec2 BR(Origin.x + mSize, Origin.y + mSize);

        const Bool IsClicked = Toolkit::Composer::InvisibleButton(String<128>::Print<"##gallery_cell_{0}">(ID), ImVec2(mSize, mSize));
        const Bool IsHovered = Toolkit::Composer::IsItemHovered();

        if (IsClicked)
        {
            mSelection = ID;
        }

        // A right-click both selects the item and flags it for opening in its dedicated editor.
        if (Toolkit::Composer::IsItemClicked(ImGuiMouseButton_Right))
        {
            mSelection = ID;
            mActivated = ID;
        }

        // Background highlight for selected or hovered state.
        const Ptr<ImDrawList> DrawList = Toolkit::Composer::GetWindowDrawList();

        if (WasSelected || IsClicked)
        {
            DrawList->AddRectFilled(Origin, BR, Toolkit::Composer::GetColorU32(ImGuiCol_ButtonActive, 0.5f));
        }
        else if (IsHovered)
        {
            DrawList->AddRectFilled(Origin, BR, Toolkit::Composer::GetColorU32(ImGuiCol_ButtonHovered, 0.5f));
        }

        // Draw the thumbnail if available, otherwise render a "?" placeholder.
        if (Thumbnail)
        {
            const ImVec2 UV0(Crop.GetMinimumX(), Crop.GetMinimumY());
            const ImVec2 UV1(Crop.GetMaximumX(), Crop.GetMaximumY());
            DrawList->AddImage(Thumbnail, Origin, BR, UV0, UV1, Tint.ToRGBA8());
        }
        else
        {
            constexpr Text kPlaceholder = "?";
            const ImVec2        TextSize     = Toolkit::Composer::CalcTextSize(kPlaceholder);
            const ImVec2        TextPos(
                Origin.x + (mSize - TextSize.x) * 0.5f,
                Origin.y + (mSize - TextSize.y) * 0.5f);
            DrawList->AddText(TextPos, Toolkit::Composer::GetColorU32(ImGuiCol_TextDisabled), kPlaceholder.GetData());
        }

        // Cell border; changes colour for selected / hovered states.
        const ImU32 BorderColor = (WasSelected || IsClicked)
            ? Toolkit::Composer::GetColorU32(ImGuiCol_ButtonActive)
            : (IsHovered
                ? Toolkit::Composer::GetColorU32(ImGuiCol_ButtonHovered)
                : Toolkit::Composer::GetColorU32(ImGuiCol_Border));
        DrawList->AddRect(Origin, BR, BorderColor);

        // Show item name as a tooltip on hover.
        if (IsHovered)
        {
            Toolkit::Composer::SetTooltip(Name);
        }

        return IsClicked;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Gallery::End()
    {
        if (mActive)
        {
            Toolkit::Composer::EndTable();
        }

        mActive = false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Gallery::Filter(Text Name) const
    {
        if (!mFilter.IsEmpty())
        {
            if (mFilter.GetSize() > Name.GetSize())
            {
                return false;
            }
            for (UInt32 I = 0; I <= Name.GetSize() - mFilter.GetSize(); ++I)
            {
                Bool Match = true;

                for (UInt32 J = 0; J < mFilter.GetSize(); ++J)
                {
                    if (StrLowercase(Name[I + J]) != StrLowercase(mFilter[J]))
                    {
                        Match = false;
                        break;
                    }
                }
                if (Match)
                {
                    return true;
                }
            }
            return false;
        }
        return true;
    }
}