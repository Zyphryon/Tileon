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

#include "Palette.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Palette::Palette(Ref<Context> Context)
        : Activity    { Context, "Palette" },
          mRepository { Context.GetRepository() },
          mTileset    { Context.GetTileset() }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::OnDraw(Ref<UI::Composer> Composer)
    {
        Composer.SetNextWindowSize(300.0f, 500.0f, ImGuiCond_FirstUseEver);
        Composer.SetNextWindowSizeConstraints(200.0f, 300.0f, 800.0f, 1200.0f);

        if (Composer.Begin(GetTitle(), mVisible))
        {
            mGallery.DrawToolbar(Composer);
            Composer.Separator();

            const Real32 BodyHeight = -(Composer.GetFrameHeightWithSpacing() + 8.0f);
            Composer.BeginChild("##body", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_None);
            DrawGallery(Composer);
            Composer.EndChild();

            DrawBottomBar(Composer);
        }
        Composer.End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawGallery(Ref<UI::Composer> Composer)
    {
        mGallery.SetSelection(GetContext().GetInteger("Selection.Tile", 0));

        mGallery.Begin(Composer);
        mRepository.ForEachTerrain([&](ConstRef<Terrain> Terrain)
        {
            ConstRef<Tileset::Glyph> Glyph = mTileset.GetGlyph(Terrain.GetID());

            Bool WasSelected;

            // Draw the terrain item in the gallery, using its material thumbnail if available.
            if (Glyph.Material && Glyph.Material->GetTexture(Graphic::TextureSemantic::Albedo))
            {
                const Graphic::Object Thumbnail = Glyph.Material->GetTexture(Graphic::TextureSemantic::Albedo)->GetID();

                WasSelected = mGallery.DrawItem(Composer,
                    Terrain.GetID(),
                    Terrain.GetName(), Thumbnail, Glyph.Crop, Glyph.Tint);
            }
            else
            {
                WasSelected = mGallery.DrawItem(Composer, Terrain.GetID(), Terrain.GetName());
            }

            // Set the selected tile in the context when the item is clicked.
            if (WasSelected)
            {
                GetContext().SetInteger("Selection.Tile", Terrain.GetID());
            }
        });
        mGallery.End(Composer);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawBottomBar(Ref<UI::Composer> Composer)
    {
        const Real32 BarHeight = Composer.GetFrameHeightWithSpacing() + 4.0f;

        Composer.PushStyleColor(ImGuiCol_ChildBg, Composer.GetStyleColorVec4(ImGuiCol_MenuBarBg));
        Composer.BeginChild("##picker_status", ImVec2(0.0f, BarHeight), ImGuiChildFlags_None);
        Composer.PopStyleColor();

        const Real32 PadY = (BarHeight - Composer.GetTextLineHeight()) * 0.5f - Composer.GetStyle().ItemSpacing.y * 0.5f;
        Composer.SetCursorPosY(PadY);

        const UInt16 Selection = GetContext().GetInteger("Selection.Tile", 0);

        if (mRepository.HasTerrain(Selection))
        {
            ConstRef<Terrain> Terrain = mRepository.GetTerrain(Selection);

            Composer.SetCursorPosX(Composer.GetStyle().ItemSpacing.x);
            Composer.Label("{:04}  {}", Selection, Terrain.GetName().empty() ? "(Unnamed)" : Terrain.GetName());
        }
        else
        {
            constexpr ConstStr8 kHint = "No terrain selected";

            Composer.SetCursorPosX((Composer.GetWindowWidth() - Composer.CalcTextSize(kHint).x) * 0.5f);
            Composer.TextDisabled(kHint);
        }

        Composer.EndChild();
    }
}


