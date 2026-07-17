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
            if (Composer.BeginTabBar("##palette_tabs"))
            {
                if (Composer.BeginTabItem("Terrain"))
                {
                    DrawTerrainTab(Composer);
                    Composer.EndTabItem();
                }

                if (Composer.BeginTabItem("Entity"))
                {
                    DrawEntityTab(Composer);
                    Composer.EndTabItem();
                }

                Composer.EndTabBar();
            }
        }
        Composer.End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainTab(Ref<UI::Composer> Composer)
    {
        mTerrains.DrawToolbar(Composer);
        Composer.Separator();

        const Real32 BodyHeight = -(Composer.GetFrameHeightWithSpacing() + 8.0f);
        Composer.BeginChild("##terrain_body", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_None);
        DrawTerrainGallery(Composer);
        Composer.EndChild();

        DrawBottomBar(Composer, "##terrain_status", [&]()
        {
            DrawTerrainStatus(Composer);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawEntityTab(Ref<UI::Composer> Composer)
    {
        mEntities.DrawToolbar(Composer);
        Composer.Separator();

        const Real32 BodyHeight = -(Composer.GetFrameHeightWithSpacing() + 8.0f);
        Composer.BeginChild("##entity_body", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_None);
        DrawEntityGallery(Composer);
        Composer.EndChild();

        DrawBottomBar(Composer, "##entity_status", [&]()
        {
            DrawEntityStatus(Composer);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainGallery(Ref<UI::Composer> Composer)
    {
        mTerrains.SetSelection(GetContext().GetInteger("Selection.Tile", 0));

        mTerrains.Begin(Composer);
        mRepository.ForEachTerrain([&](ConstRef<Terrain> Terrain)
        {
            ConstRef<Tileset::Glyph> Glyph = mTileset.GetGlyph(Terrain.GetID());

            Bool WasSelected;

            // Draw the terrain item in the gallery, using its material thumbnail if available.
            if (Glyph.Material && Glyph.Material->GetImage(Graphic::TextureSlot::Albedo))
            {
                const Graphic::Object Thumbnail = Glyph.Material->GetImage(Graphic::TextureSlot::Albedo)->GetHandle();

                WasSelected = mTerrains.DrawItem(Composer,
                    Terrain.GetID(),
                    Terrain.GetName(), Thumbnail, Glyph.Crop, Glyph.Tint);
            }
            else
            {
                WasSelected = mTerrains.DrawItem(Composer, Terrain.GetID(), Terrain.GetName());
            }

            // Set the selected tile in the context when the item is clicked.
            if (WasSelected)
            {
                GetContext().SetInteger("Selection.Tile", Terrain.GetID());
            }
        });
        mTerrains.End(Composer);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawEntityGallery(Ref<UI::Composer> Composer)
    {
        mEntities.SetSelection(GetContext().GetInteger("Selection.Archetype", 0));

        mEntities.Begin(Composer);
        mRepository.ForEachArchetype([&](Scene::Entity Archetype)
        {
            const UInt32 ID = static_cast<UInt32>(Archetype.GetID() - Scene::kMinRangeArchetypes);

            Graphic::Object Thumbnail = 0;
            Rect            Crop      = Rect::One();
            IntColor8       Tint      = IntColor8::White();

            if (const ConstPtr<Appearance> Visual = Archetype.TryGet<const Appearance>())
            {
                ConstRetainer<Graphic::Material> Material = Visual->GetMaterial();

                if (Material && Material->HasCompleted())
                {
                    if (ConstRetainer<Graphic::Image> Albedo = Material->GetImage(Graphic::TextureSlot::Albedo))
                    {
                        Thumbnail = Albedo->GetHandle();
                        Crop      = Visual->GetSource();

                        if (const ConstPtr<IntColor8> Color = Archetype.TryGet<const IntColor8>())
                        {
                            Tint = (* Color);
                        }
                    }
                }
            }

            // An archetype that is still loading, failed, or has no sprite falls back to the gallery's placeholder.
            if (mEntities.DrawItem(Composer, ID, Archetype.GetAlias(), Thumbnail, Crop, Tint))
            {
                GetContext().SetInteger("Selection.Archetype", ID);
            }
        });
        mEntities.End(Composer);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainStatus(Ref<UI::Composer> Composer)
    {
        const UInt16 Selection = GetContext().GetInteger("Selection.Tile", 0);

        if (mRepository.HasTerrain(Selection))
        {
            ConstRef<Terrain> Terrain = mRepository.GetTerrain(Selection);

            Composer.SetCursorPosX(Composer.GetStyle().ItemSpacing.x);
            Composer.Label("{0:04}  {1}", Selection, Terrain.GetName().IsEmpty() ? "(Unnamed)" : Terrain.GetName());
        }
        else
        {
            DrawHint(Composer, "No terrain selected");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawEntityStatus(Ref<UI::Composer> Composer)
    {
        const UInt32        Selection = GetContext().GetInteger("Selection.Archetype", 0);
        const Scene::Entity Archetype = mRepository.GetArchetype(Scene::kMinRangeArchetypes + Selection);

        if (Selection != 0 && Archetype.IsValid())
        {
            const Text Alias = Archetype.GetAlias();

            Composer.SetCursorPosX(Composer.GetStyle().ItemSpacing.x);
            Composer.Label("{0:04}  {1}", Selection, Alias.IsEmpty() ? "(Unnamed)" : Alias);
        }
        else
        {
            DrawHint(Composer, "No archetype selected");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawHint(Ref<UI::Composer> Composer, Text Hint)
    {
        Composer.SetCursorPosX((Composer.GetWindowWidth() - Composer.CalcTextSize(Hint).x) * 0.5f);
        Composer.TextDisabled(Hint);
    }
}


