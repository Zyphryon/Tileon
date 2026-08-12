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
#include "Tileon.Editor/Activity/Atelier/Workshop.hpp"
#include "Tileon.Editor/Component/Texture.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Palette::Palette(Ref<Context> Context)
        : Activity    { Context, "Palette", true },
          mRepository { Context.GetRepository() },
          mTileset    { Context.GetTileset() },
          mMode       { -1 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::OnDraw()
    {
        Toolkit::Composer::SetNextWindowSize(300.0f, 500.0f, ImGuiCond_FirstUseEver);
        Toolkit::Composer::SetNextWindowSizeConstraints(200.0f, 300.0f, 800.0f, 1200.0f);

        if (Toolkit::Composer::Begin(GetTitle(), mVisible))
        {
            const Workshop::Mode Mode     = GetContext().GetEnum("Workshop.Mode", Workshop::Mode::Tile);
            const Bool           External = (static_cast<SInt32>(Mode) != mMode);

            if (Toolkit::Composer::BeginTabBar("##palette_tabs"))
            {
                const ImGuiTabItemFlags TerrainFlags =
                    (External && Mode == Workshop::Mode::Tile)
                        ? ImGuiTabItemFlags_SetSelected
                        : ImGuiTabItemFlags_None;

                if (Toolkit::Composer::BeginTabItem("Terrain", TerrainFlags))
                {
                    if (Mode != Workshop::Mode::Tile)
                    {
                        GetContext().SetEnum("Workshop.Mode", Workshop::Mode::Tile);
                    }

                    DrawTerrainTab();
                    Toolkit::Composer::EndTabItem();
                }

                const ImGuiTabItemFlags EntityFlags =
                    (External && Mode == Workshop::Mode::Entity)
                        ? ImGuiTabItemFlags_SetSelected
                        : ImGuiTabItemFlags_None;

                if (Toolkit::Composer::BeginTabItem("Entity", EntityFlags))
                {
                    if (Mode != Workshop::Mode::Entity)
                    {
                        GetContext().SetEnum("Workshop.Mode", Workshop::Mode::Entity);
                    }

                    DrawEntityTab();
                    Toolkit::Composer::EndTabItem();
                }

                Toolkit::Composer::EndTabBar();
            }

            mMode = static_cast<SInt32>(GetContext().GetEnum("Workshop.Mode", Workshop::Mode::Tile));
        }
        Toolkit::Composer::End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainTab()
    {
        mTerrains.DrawToolbar();
        Toolkit::Composer::Separator();

        const Real32 BodyHeight = -(Toolkit::Composer::GetFrameHeightWithSpacing() + 8.0f);
        Toolkit::Composer::BeginChild("##terrain_body", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_None);
        DrawTerrainGallery();
        Toolkit::Composer::EndChild();

        DrawBottomBar("##terrain_status", [&]()
        {
            DrawTerrainStatus();
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawEntityTab()
    {
        mEntities.DrawToolbar();
        Toolkit::Composer::Separator();

        const Real32 BodyHeight = -(Toolkit::Composer::GetFrameHeightWithSpacing() + 8.0f);
        Toolkit::Composer::BeginChild("##entity_body", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_None);
        DrawEntityGallery();
        Toolkit::Composer::EndChild();

        DrawBottomBar("##entity_status", [&]
        {
            DrawEntityStatus();
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainGallery()
    {
        mTerrains.SetSelection(GetContext().GetInteger("Selection.Tile", 0));

        mTerrains.Begin();
        mRepository.ForEachTerrain([&](ConstRef<Terrain> Terrain)
        {
            ConstRef<Tileset::Glyph> Glyph = mTileset.GetGlyph(Terrain.GetID());

            Bool WasSelected;

            // Draw the terrain item in the gallery, showing the array slice its motif was promoted into.
            if (Glyph.Texture)
            {
                // Every slice shares one identifier, so which of them is drawn rides in the coordinates.
                const ImTextureID Thumbnail = Plugin::ImGuiRenderer::GetLayeredTextureID(Glyph.Texture);
                const Real32      Offset    = Glyph.Slice * Plugin::ImGuiRenderer::kSliceStride;

                WasSelected = mTerrains.DrawItem(
                    Terrain.GetID(),
                    Terrain.GetName(), Thumbnail, Rect(Offset, 0.0f, Offset + 1.0f, 1.0f), Glyph.Tint);
            }
            else
            {
                WasSelected = mTerrains.DrawItem(Terrain.GetID(), Terrain.GetName());
            }

            // Set the selected tile in the context when the item is clicked.
            if (WasSelected)
            {
                GetContext().SetInteger("Selection.Tile", Terrain.GetID());
            }
        });
        mTerrains.End();

        // Right-clicking a terrain jumps to the Foundry with it selected, keeping the palette and editor in sync.
        if (const SInt64 Target = mTerrains.GetActivated(); Target >= 0)
        {
            GetContext().SetString("Navigate.Panel", "Foundry");
            GetContext().SetInteger("Selection.Tile", Target);
            GetContext().SetInteger("Selection.Tile.Target", Target);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawEntityGallery()
    {
        mEntities.SetSelection(GetContext().GetInteger("Selection.Archetype", 0));

        mEntities.Begin();
        mRepository.ForEachArchetype([&](Scene::Entity Archetype)
        {
            // Only root archetypes are placeable.
            if (Archetype.GetParent(Scene::Hierarchy::Fixed).IsValid())
            {
                return;
            }

            const UInt32 ID = static_cast<UInt32>(Archetype.GetID() - Scene::kMinRangeArchetypes);

            Graphic::Object Thumbnail = 0;
            Rect            Crop      = Rect::One();
            IntColor8       Tint      = IntColor8::White();

            if (const ConstPtr<Appearance> Visual = Archetype.TryGet<const Appearance>())
            {
                ConstRetainer<Graphic::Material> Material = Visual->GetMaterial();

                if (Material && Material->HasCompleted())
                {
                    if (ConstRetainer<Graphic::Image> Albedo = Material->GetImage(GetTextureID(Texture::Albedo)))
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
            if (mEntities.DrawItem(ID, Archetype.GetAlias(), Thumbnail, Crop, Tint))
            {
                GetContext().SetInteger("Selection.Archetype", ID);
            }
        });
        mEntities.End();

        // Right-clicking an entity jumps to the Archetypes editor with it selected, keeping palette and editor in sync.
        if (const SInt64 Target = mEntities.GetActivated(); Target >= 0)
        {
            GetContext().SetString("Navigate.Panel", "Archetypes");
            GetContext().SetInteger("Selection.Archetype", Target);
            GetContext().SetInteger("Selection.Archetype.Target", Target);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainStatus()
    {
        const UInt16 Selection = GetContext().GetInteger("Selection.Tile", 0);

        if (mRepository.HasTerrain(Selection))
        {
            ConstRef<Terrain> Terrain = mRepository.GetTerrain(Selection);

            Toolkit::Composer::SetCursorPosX(Toolkit::Composer::GetStyle().ItemSpacing.x);
            Toolkit::Composer::Label("{0:04}  {1}", Selection, Terrain.GetName().IsEmpty() ? "(Unnamed)" : Terrain.GetName());
        }
        else
        {
            DrawHint("No terrain selected");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawEntityStatus()
    {
        const UInt32        Selection = GetContext().GetInteger("Selection.Archetype", 0);
        const Scene::Entity Archetype = mRepository.GetArchetype(Scene::kMinRangeArchetypes + Selection);

        if (Selection != 0 && Archetype.IsValid())
        {
            const Text Alias = Archetype.GetAlias();

            Toolkit::Composer::SetCursorPosX(Toolkit::Composer::GetStyle().ItemSpacing.x);
            Toolkit::Composer::Label("{0:04}  {1}", Selection, Alias.IsEmpty() ? "(Unnamed)" : Alias);
        }
        else
        {
            DrawHint("No archetype selected");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawHint(Text Hint)
    {
        Toolkit::Composer::SetCursorPosX((Toolkit::Composer::GetWindowWidth() - Toolkit::Composer::CalcTextSize(Hint).x) * 0.5f);
        Toolkit::Composer::TextDisabled(Hint);
    }
}