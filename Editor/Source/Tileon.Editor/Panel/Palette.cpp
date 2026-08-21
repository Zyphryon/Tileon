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
#include "Tileon.Render/Types.hpp"
#include "Tileon.Editor/Utility.hpp"
#include "Tileon.Editor/Edit/Tools.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Palette::Palette(Ref<Context> Context)
        : Panel       { Context, "Palette", true },
          mRepository { Context.GetRepository() },
          mSplatset   { Context.GetRenderer().GetSplatset() },
          mAssembler  { Context, Context.GetRenderer().GetSplatset() },
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
            const Tools::Mode Mode     = GetContext().GetEnum("Tools.Mode", Tools::Mode::Ground);
            const Bool        External = (static_cast<SInt32>(Mode) != mMode);

            if (Toolkit::Composer::BeginTabBar("##palette_tabs"))
            {
                const ImGuiTabItemFlags TerrainFlags =
                    (External && Mode != Tools::Mode::Entity)
                        ? ImGuiTabItemFlags_SetSelected
                        : ImGuiTabItemFlags_None;

                if (Toolkit::Composer::BeginTabItem("Terrain", TerrainFlags))
                {
                    if (!External && Mode == Tools::Mode::Entity)
                    {
                        GetContext().SetEnum("Tools.Mode", Tools::Mode::Ground);
                    }

                    DrawTerrainTab();
                    Toolkit::Composer::EndTabItem();
                }

                const ImGuiTabItemFlags EntityFlags =
                    (External && Mode == Tools::Mode::Entity)
                        ? ImGuiTabItemFlags_SetSelected
                        : ImGuiTabItemFlags_None;

                if (Toolkit::Composer::BeginTabItem("Entity", EntityFlags))
                {
                    if (!External && Mode != Tools::Mode::Entity)
                    {
                        GetContext().SetEnum("Tools.Mode", Tools::Mode::Entity);
                    }

                    DrawEntityTab();
                    Toolkit::Composer::EndTabItem();
                }

                Toolkit::Composer::EndTabBar();
            }

            mMode = static_cast<SInt32>(GetContext().GetEnum("Tools.Mode", Tools::Mode::Ground));
        }
        Toolkit::Composer::End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainTab()
    {
        DrawTerrainAuthor();

        mTerrains.DrawToolbar();
        Toolkit::Composer::Separator();

        const Real32 BodyHeight = -(Toolkit::Composer::GetFrameHeightWithSpacing() + 8.0f);
        Toolkit::Composer::BeginChild("##terrain_body", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_None);
        DrawTerrainGallery();
        Toolkit::Composer::EndChild();

        DrawTerrainProperties();

        DrawBottomBar("##terrain_status", [&](Real32)
        {
            DrawTerrainStatus();
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainAuthor()
    {
        static constexpr UInt64 kAlbedoKey = "Palette.Terrain.Albedo"_Hash;
        static constexpr UInt64 kNormalKey = "Palette.Terrain.Normal"_Hash;

        Ref<Toolkit::Browser> Browser = GetContext().GetBrowser();

        if (Str Selection; Browser.Consume(kAlbedoKey, Selection))
        {
            mPendingAlbedo = Move(Selection);
        }
        if (Str Selection; Browser.Consume(kNormalKey, Selection))
        {
            mPendingNormal = Move(Selection);
        }

        const auto DrawSource = [&](Text Label, UInt64 Key, Ref<Str> Value, Text Hint)
        {
            Toolkit::Composer::FieldInline(Label);
            Toolkit::Composer::PushID(Label);
            Toolkit::Composer::InputTextWithButton("##art", Value,
                [&](Text Path)
                {
                    Value = Path;
                },
                ICON_FA_ELLIPSIS,
                [&]
                {
                    Browser.Open(Key, ".png .jpg .tga .bmp .tex");
                },
                ImGuiInputTextFlags_EnterReturnsTrue);
            Toolkit::Composer::PopID();
            Toolkit::Composer::Tooltip(Hint);
        };

        DrawSource("Colour", kAlbedoKey, mPendingAlbedo,
            "The art the terrain is coloured by, which becomes one slice of the tileset."_Text);
        DrawSource("Relief", kNormalKey, mPendingNormal,
            "Its normal map, taken at the same slice, which a project may go without."_Text);

        Toolkit::Composer::FieldInline("Name"_Text);
        Toolkit::Composer::InputText("##name", mPendingName, [&](Text Value)
        {
            mPendingName = Value;
        });
        Toolkit::Composer::Tooltip("What to call the terrain, which nothing but the palette reads."_Text);

        // The arrays are indexed by the same slice, so one may not grow without the other.
        const Bool Missing = mAssembler.GetArray(Texture::Normal) && mPendingNormal.IsEmpty();

        Toolkit::Composer::BeginDisabled(mPendingAlbedo.IsEmpty() || Missing);

        if (Toolkit::Composer::Button(ICON_FA_PLUS " Add Terrain"))
        {
            if (mAssembler.Append(mPendingAlbedo, mPendingNormal, mPendingName))
            {
                mPendingAlbedo.Clear();
                mPendingNormal.Clear();
                mPendingName.Clear();
            }
        }
        Toolkit::Composer::EndDisabled();

        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::Button(ICON_FA_MINUS " Remove Terrain"))
        {
            mSplatset.RemoveTerrain(static_cast<UInt16>(GetContext().GetInteger("Selection.Terrain", 0)));
        }
        Toolkit::Composer::Tooltip(
            "Takes the terrain out of the palette. Its art stays in the array, since every painted region "
            "names its art by slice, so whatever was painted with it goes on drawing."_Text);

        if (Missing)
        {
            Toolkit::Composer::Tooltip(
                "This tileset carries relief, so a terrain added without a normal map would read "
                "somebody else's."_Text);
        }
        Toolkit::Composer::Separator();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::OnCommit()
    {
        mAssembler.Commit();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainProperties()
    {
        const UInt16 Selection = static_cast<UInt16>(GetContext().GetInteger("Selection.Terrain", 0));

        if (!mAssembler.GetArray(Texture::Albedo) || Selection >= mSplatset.GetTerrains().GetSize())
        {
            return;
        }

        Ref<Splatset::Terrain> Terrain = mSplatset.GetTerrain(Selection);

        Toolkit::Composer::Section("Terrain");

        Toolkit::Composer::FieldInline("Name");
        Toolkit::Composer::PushID("Name");
        Toolkit::Composer::InputText("##value", Terrain.Name, [&](Text Value)
        {
            Terrain.Name = Value;
        });
        Toolkit::Composer::PopID();

        Toolkit::Composer::FieldInline("Tiling");
        Toolkit::Composer::PushID("Tiling");

        if (Real32 Tiling = Terrain.Tiling; Toolkit::Composer::InputFloat("##value", Tiling, 0.05f, 0.25f, "%.3f"))
        {
            Terrain.Tiling = Clamp(Tiling, 0.01f, 64.0f);
        }
        Toolkit::Composer::PopID();

        Toolkit::Composer::FieldInline("Tint");
        Toolkit::Composer::PushID("Tint");
        Toolkit::Composer::InputTintSmall("##value", Terrain.Tint);
        Toolkit::Composer::PopID();

        static constexpr UInt64 kReliefKey = "Palette.Terrain.Relief"_Hash;

        Ref<Toolkit::Browser> Browser = GetContext().GetBrowser();

        if (Str Picked; Browser.Consume(kReliefKey, Picked))
        {
            mAssembler.AppendRelief(Selection, Picked);
        }

        const Bool Mounted = (mAssembler.GetArray(Texture::Normal) != nullptr);

        Toolkit::Composer::FieldInline("Relief");
        Toolkit::Composer::PushID("Relief");
        Toolkit::Composer::BeginDisabled(!Mounted);

        if (Toolkit::Composer::Button(Terrain.Relief
                ? ICON_FA_CHECK " Authored"_Text
                : ICON_FA_ELLIPSIS " Set normal map"_Text))
        {
            Browser.Open(kReliefKey, ".png .jpg .tga .bmp .tex");
        }
        Toolkit::Composer::EndDisabled();
        Toolkit::Composer::PopID();

        Toolkit::Composer::Tooltip(!Mounted
            ? "This project has no normal array. Author a terrain with one to bring it about."_Text
            : Terrain.Relief
                ? "The relief this terrain is lit by. Choose again to replace it."_Text
                : "This terrain is lit flat. Choose the normal map it should wear."_Text);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainGallery()
    {
        mTerrains.SetSelection(static_cast<UInt32>(GetContext().GetInteger("Selection.Terrain", 0)));

        ConstRetainer<Graphic::Image> Array = mAssembler.GetArray(Texture::Albedo);

        if (!Array)
        {
            DrawHint("No terrain array loaded");
            return;
        }

        const ImTextureID Thumbnail = Plugin::ImGuiRenderer::GetLayeredTextureID(Array->GetHandle());

        mTerrains.Begin();

        const ConstSpan<Splatset::Terrain> Terrains = mSplatset.GetTerrains();

        for (UInt16 Slice = 0; Slice < Terrains.GetSize(); ++Slice)
        {
            ConstRef<Splatset::Terrain> Terrain = Terrains[Slice];

            if (Terrain.Retired)
            {
                continue;
            }

            const ImVec2 Minimum = Plugin::ImGuiRenderer::GetLayeredTextureUV(Slice, ImVec2(0.0f, 0.0f));
            const ImVec2 Maximum = Plugin::ImGuiRenderer::GetLayeredTextureUV(Slice, ImVec2(1.0f, 1.0f));

            if (mTerrains.DrawItem(
                    Slice,
                    Terrain.Name.IsEmpty() ? Str::Print<"{0}">(Slice) : Str(Terrain.Name),
                    Thumbnail,
                    Rect(Minimum.x, Minimum.y, Maximum.x, Maximum.y),
                    IntColor8::White()))
            {
                GetContext().SetInteger("Selection.Terrain", Slice);
            }
        }
        mTerrains.End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainStatus()
    {
        const UInt16 Selection = static_cast<UInt16>(GetContext().GetInteger("Selection.Terrain", 0));

        const ConstSpan<Splatset::Terrain> Terrains = mSplatset.GetTerrains();

        if (Selection >= Terrains.GetSize() || Terrains[Selection].Retired)
        {
            DrawHint(Terrains.GetSize() == 0 ? "No terrain array loaded"_Text : "No terrain selected"_Text);
            return;
        }

        ConstRef<Str> Name = Terrains[Selection].Name;

        Toolkit::Composer::SetCursorPosX(Toolkit::Composer::GetStyle().ItemSpacing.x);
        Toolkit::Composer::Label("{0}  {1} of {2}", Name.IsEmpty() ? "(Unnamed)" : Name, Selection, Terrains.GetSize());
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

        DrawBottomBar("##entity_status", [&](Real32)
        {
            DrawEntityStatus();
        });
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
            GetContext().SetInteger("Selection.Archetype.Target", Scene::kMinRangeArchetypes + Target);
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
        Toolkit::Composer::SetCursorPosX(
            (Toolkit::Composer::GetWindowWidth() - Toolkit::Composer::CalcTextSize(Hint).x) * 0.5f);
        Toolkit::Composer::TextDisabled(Hint);
    }
}