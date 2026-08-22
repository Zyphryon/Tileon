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

#include "Tileon.Editor/Toolkit/Gallery.hpp"

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
        const Real32 Spacing = Composer::GetStyle().ItemSpacing.x;
        const Real32 Padding = Composer::GetStyle().FramePadding.x;

        // Pre-compute widths of the fixed controls.
        const Real32 ListBtnW  = Composer::CalcTextSize(ICON_FA_LIST).x              + Padding * 2.0f;
        const Real32 GridBtnW  = Composer::CalcTextSize(ICON_FA_TABLE_CELLS_LARGE).x + Padding * 2.0f;
        const Real32 SliderW   = (mMode == Mode::Grid) ? 80.0f : 0.0f;
        const Real32 SliderGap = (mMode == Mode::Grid) ? (Spacing + SliderW) : 0.0f;

        // Filter input stretches to fill remaining space.
        const Real32 FilterW = Composer::GetContentRegionAvail().x - ListBtnW - GridBtnW - SliderGap - Spacing * 2.0f;

        Composer::SetNextItemWidth(FilterW > 0.0f ? FilterW : 1.0f);
        Composer::InputText("##gallery_filter", mFilter, [this](Text Value)
        {
            mFilter = Value;
        });

        Composer::SameLine();

        // List mode toggle button.
        if (Composer::ToggleButton(ICON_FA_LIST, mMode == Mode::List))
        {
            mMode = Mode::List;
        }
        Composer::SameLine();

        // Grid mode toggle button.
        if (Composer::ToggleButton(ICON_FA_TABLE_CELLS_LARGE, mMode == Mode::Grid))
        {
            mMode = Mode::Grid;
        }
        // Cell-size slider, visible in grid mode only.
        if (mMode == Mode::Grid)
        {
            Composer::SameLine();
            Composer::SetNextItemWidth(SliderW);
            Composer::SliderFloat("##gallery_cellsize", mSize, kThumbnailMinSize, kThumbnailMaxSize, "%.0f");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Gallery::Begin()
    {
        // Right-click activation is a per-frame, one-shot signal, so clear it before the items are drawn.
        mActivated = -1;

        mCells.Clear();

        if (mMode == Mode::Grid)
        {
            const ImVec2 Available = Composer::GetContentRegionAvail();
            const Real32 Spacing   = Composer::GetStyle().ItemSpacing.x;
            const SInt32 Columns   = Max(1, static_cast<SInt32>((Available.x + Spacing) / (mSize + Spacing)));

            mActive = Composer::BeginTable("##gallery", Columns);

            if (mActive)
            {
                for (SInt32 I = 0; I < Columns; ++I)
                {
                    Composer::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, mSize);
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

            const Bool Chosen = Composer::Selectable(Label, WasSelected);

            // A right-click both selects the item and flags it for opening in its dedicated editor.
            if (Composer::IsItemClicked(ImGuiMouseButton_Right))
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

        Composer::TableNextColumn();

        // A cell scrolled out of view still costs an id, a hit test and a textured draw command, and a
        // thumbnail of its own defeats batching, so the row is only reserved and nothing is submitted.
        if (!Composer::IsRectVisible(ImVec2(mSize, mSize)))
        {
            Composer::Dummy(ImVec2(mSize, mSize));

            return false;
        }

        const ImVec2 Origin    = Composer::GetCursorScreenPos();
        const ImVec2 BR(Origin.x + mSize, Origin.y + mSize);

        const Bool IsClicked = Composer::InvisibleButton(String<128>::Print<"##gallery_cell_{0}">(ID), ImVec2(mSize, mSize));
        const Bool IsHovered = Composer::IsItemHovered();

        if (IsClicked)
        {
            mSelection = ID;
        }

        // A right-click both selects the item and flags it for opening in its dedicated editor.
        if (Composer::IsItemClicked(ImGuiMouseButton_Right))
        {
            mSelection = ID;
            mActivated = ID;
        }

        Ref<Cell> Record = mCells.Append();
        Record.Minimum = Origin;
        Record.Maximum = BR;
        Record.First   = ImVec2(Crop.GetMinimumX(), Crop.GetMinimumY());
        Record.Last    = ImVec2(Crop.GetMaximumX(), Crop.GetMaximumY());
        Record.Texture = Thumbnail;
        Record.Tint    = Tint.ToRGBA8();

        // Background highlight for selected or hovered state.
        if (WasSelected || IsClicked)
        {
            Record.Fill = Composer::GetColorU32(ImGuiCol_ButtonActive, 0.5f);
        }
        else if (IsHovered)
        {
            Record.Fill = Composer::GetColorU32(ImGuiCol_ButtonHovered, 0.5f);
        }

        // Cell border; changes colour for selected / hovered states.
        Record.Border = (WasSelected || IsClicked)
            ? Composer::GetColorU32(ImGuiCol_ButtonActive)
            : (IsHovered
                ? Composer::GetColorU32(ImGuiCol_ButtonHovered)
                : Composer::GetColorU32(ImGuiCol_Border));

        // Show item name as a tooltip on hover.
        if (IsHovered)
        {
            Composer::SetTooltip(Name);
        }

        return IsClicked;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Gallery::End()
    {
        if (mActive)
        {
            Composer::EndTable();
        }

        Flush();

        mActive = false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Gallery::Flush()
    {
        if (mCells.IsEmpty())
        {
            return;
        }

        const Ptr<ImDrawList> DrawList = Composer::GetWindowDrawList();

        mCells.Sort([](ConstRef<Cell> Left, ConstRef<Cell> Right)
        {
            return Left.Texture < Right.Texture;
        });

        for (ConstRef<Cell> Item : mCells)
        {
            if (Item.Fill)
            {
                DrawList->AddRectFilled(Item.Minimum, Item.Maximum, Item.Fill);
            }
        }

        for (ConstRef<Cell> Item : mCells)
        {
            if (Item.Texture)
            {
                DrawList->AddImage(Item.Texture, Item.Minimum, Item.Maximum, Item.First, Item.Last, Item.Tint);
            }
        }

        constexpr Text kPlaceholder = "?";

        const ImVec2 TextSize  = Composer::CalcTextSize(kPlaceholder);
        const ImU32  TextColor = Composer::GetColorU32(ImGuiCol_TextDisabled);

        for (ConstRef<Cell> Item : mCells)
        {
            DrawList->AddRect(Item.Minimum, Item.Maximum, Item.Border);

            // A cell with nothing to show says so, centred where its thumbnail would have been.
            if (!Item.Texture)
            {
                const ImVec2 TextPos(
                    Item.Minimum.x + (mSize - TextSize.x) * 0.5f,
                    Item.Minimum.y + (mSize - TextSize.y) * 0.5f);

                DrawList->AddText(TextPos, TextColor, kPlaceholder.GetData());
            }
        }

        mCells.Clear();
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