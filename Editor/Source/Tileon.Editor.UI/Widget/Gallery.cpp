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

namespace Tileon::Editor::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Gallery::Gallery()
        : mActive     { false },
          mMode       { Mode::Grid },
          mSize       { 64.0f },
          mSelection  { 0 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Gallery::DrawToolbar(Ref<Composer> Composer)
    {
        const Real32 Spacing = Composer.GetStyle().ItemSpacing.x;
        const Real32 Padding = Composer.GetStyle().FramePadding.x;

        // Pre-compute widths of the fixed controls.
        const Real32 ListBtnW  = Composer.CalcTextSize(ICON_FA_LIST).x              + Padding * 2.0f;
        const Real32 GridBtnW  = Composer.CalcTextSize(ICON_FA_TABLE_CELLS_LARGE).x + Padding * 2.0f;
        const Real32 SliderW   = (mMode == Mode::Grid) ? 80.0f : 0.0f;
        const Real32 SliderGap = (mMode == Mode::Grid) ? (Spacing + SliderW) : 0.0f;

        // Filter input stretches to fill remaining space.
        const Real32 FilterW = Composer.GetContentRegionAvail().x - ListBtnW - GridBtnW - SliderGap - Spacing * 2.0f;

        Composer.SetNextItemWidth(FilterW > 0.0f ? FilterW : 1.0f);
        Composer.InputText("##gallery_filter", mFilter, [this](ConstStr8 Value)
        {
            mFilter = Value;
        });

        Composer.SameLine();

        // List mode toggle button.
        const Bool ListActive = (mMode == Mode::List);
        if (ListActive)
        {
            Composer.PushStyleColor(ImGuiCol_Button,        Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
            Composer.PushStyleColor(ImGuiCol_ButtonHovered, Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        if (Composer.Button(ICON_FA_LIST))
        {
            mMode = Mode::List;
        }
        if (ListActive)
        {
            Composer.PopStyleColor(2);
        }

        Composer.SameLine();

        // Grid mode toggle button.
        const Bool GridActive = (mMode == Mode::Grid);
        if (GridActive)
        {
            Composer.PushStyleColor(ImGuiCol_Button,        Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
            Composer.PushStyleColor(ImGuiCol_ButtonHovered, Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        if (Composer.Button(ICON_FA_TABLE_CELLS_LARGE))
        {
            mMode = Mode::Grid;
        }
        if (GridActive)
        {
            Composer.PopStyleColor(2);
        }

        // Cell-size slider, visible in grid mode only.
        if (mMode == Mode::Grid)
        {
            Composer.SameLine();
            Composer.SetNextItemWidth(SliderW);
            Composer.SliderFloat("##gallery_cellsize", mSize, kThumbnailMinSize, kThumbnailMaxSize, "%.0f");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Gallery::Begin(Ref<Composer> Composer)
    {
        if (mMode == Mode::Grid)
        {
            const ImVec2 Available = Composer.GetContentRegionAvail();
            const Real32 Spacing   = Composer.GetStyle().ItemSpacing.x;
            const SInt32 Columns   = Max(1, static_cast<SInt32>((Available.x + Spacing) / (mSize + Spacing)));

            mActive = Composer.BeginTable("##gallery", Columns);

            if (mActive)
            {
                for (SInt32 I = 0; I < Columns; ++I)
                {
                    Composer.TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, mSize);
                }
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Gallery::DrawItem(Ref<Composer> Composer, UInt32 ID, ConstStr8 Name, Graphic::Object Thumbnail, Rect Crop, IntColor8 Tint)
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
            const ConstStr8 Label = Base::Format("{}##{}", Name, ID);

            if (Composer.Selectable(Label, WasSelected))
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

        Composer.TableNextColumn();

        const ImVec2 Origin    = Composer.GetCursorScreenPos();
        const ImVec2 BR(Origin.x + mSize, Origin.y + mSize);

        const Bool IsClicked = Composer.InvisibleButton(Base::Format("##gallery_cell_{}", ID), ImVec2(mSize, mSize));
        const Bool IsHovered = Composer.IsItemHovered();

        if (IsClicked)
        {
            mSelection = ID;
        }

        // Background highlight for selected or hovered state.
        const Ptr<ImDrawList> DrawList = Composer.GetWindowDrawList();

        if (WasSelected || IsClicked)
        {
            DrawList->AddRectFilled(Origin, BR, Composer.GetColorU32(ImGuiCol_ButtonActive, 0.5f));
        }
        else if (IsHovered)
        {
            DrawList->AddRectFilled(Origin, BR, Composer.GetColorU32(ImGuiCol_ButtonHovered, 0.5f));
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
            constexpr ConstStr8 kPlaceholder = "?";
            const ImVec2        TextSize     = Composer.CalcTextSize(kPlaceholder);
            const ImVec2        TextPos(
                Origin.x + (mSize - TextSize.x) * 0.5f,
                Origin.y + (mSize - TextSize.y) * 0.5f);
            DrawList->AddText(TextPos, Composer.GetColorU32(ImGuiCol_TextDisabled), kPlaceholder.data());
        }

        // Cell border; changes colour for selected / hovered states.
        const ImU32 BorderColor = (WasSelected || IsClicked)
            ? Composer.GetColorU32(ImGuiCol_ButtonActive)
            : (IsHovered
                ? Composer.GetColorU32(ImGuiCol_ButtonHovered)
                : Composer.GetColorU32(ImGuiCol_Border));
        DrawList->AddRect(Origin, BR, BorderColor);

        // Show item name as a tooltip on hover.
        if (IsHovered)
        {
            Composer.SetTooltip(Name);
        }

        return IsClicked;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Gallery::End(Ref<Composer> Composer)
    {
        if (mActive)
        {
            Composer.EndTable();
        }

        mActive = false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Gallery::Filter(ConstStr8 Name) const
    {
        if (!mFilter.empty())
        {
            if (mFilter.size() > Name.size())
            {
                return false;
            }
            for (UInt32 I = 0; I <= Name.size() - mFilter.size(); ++I)
            {
                Bool Match = true;

                for (UInt32 J = 0; J < mFilter.size(); ++J)
                {
                    if (SDL_tolower(static_cast<unsigned char>(Name[I + J])) !=
                        SDL_tolower(static_cast<unsigned char>(mFilter[J])))
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
