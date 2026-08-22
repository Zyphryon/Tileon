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

#include "Terrain.hpp"
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

    Terrain::Terrain(Ref<Context> Context)
        : Panel      { Context, "Terrains", true },
          mSplatset  { Context.GetRenderer().GetSplatset() },
          mAssembler { Context, Context.GetRenderer().GetSplatset() }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrain::OnDraw()
    {
        Toolkit::Composer::SetNextWindowSize(1000.0f, 560.0f, ImGuiCond_FirstUseEver);
        Toolkit::Composer::SetNextWindowSizeConstraints(560.0f, 340.0f, 1800.0f, 1400.0f);

        if (Toolkit::Composer::Begin(GetTitle(), mVisible))
        {
            const Real32 BodyHeight = -(Toolkit::Composer::GetFrameHeightWithSpacing() + 8.0f);

            Toolkit::Composer::BeginChild("##list_panel", ImVec2(260.0f, BodyHeight), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders);
            DrawListPanel();
            Toolkit::Composer::EndChild();

            const UInt16 Selection = static_cast<UInt16>(GetContext().GetInteger(Session::kSelectionTerrain, 0));

            if (Selection < mSplatset.GetTerrains().GetSize())
            {
                Toolkit::Composer::SameLine();
                Toolkit::Composer::BeginChild("##details_panel", ImVec2(360.0f, BodyHeight), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders);
                DrawProperties();
                DrawSettings();
                Toolkit::Composer::EndChild();

                Toolkit::Composer::SameLine();
                Toolkit::Composer::BeginChild("##preview_panel", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_Borders);
                DrawPreviewPanel();
                Toolkit::Composer::EndChild();
            }
            else
            {
                Toolkit::Composer::SameLine();
                Toolkit::Composer::BeginChild("##empty_panel", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_Borders);
                DrawEmptyPanel("No terrain selected", ICON_FA_MOUNTAIN);
                Toolkit::Composer::EndChild();
            }

            DrawStatusBar();
        }
        Toolkit::Composer::End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrain::DrawListPanel()
    {
        const ConstSpan<Splatset::Terrain> Terrains = mSplatset.GetTerrains();
        const UInt16 Selection = static_cast<UInt16>(GetContext().GetInteger(Session::kSelectionTerrain, 0));

        if (Toolkit::Composer::Button(ICON_FA_PLUS "  Terrain", -1.0f))
        {
            if (const UInt16 Added = mAssembler.Create(); Added != Splatset::kInvalid)
            {
                GetContext().SetInteger(Session::kSelectionTerrain, Added);
            }
        }

        Toolkit::Composer::Separator();
        Toolkit::Composer::BeginChild("##list_scroll");

        for (UInt16 Slice = 0; Slice < Terrains.GetSize(); ++Slice)
        {
            ConstRef<Splatset::Terrain> Terrain = Terrains[Slice];

            if (Terrain.Retired)
            {
                continue;
            }

            const Str Label = Terrain.Name.IsEmpty()
                ? Str::Print<"{0}  Terrain {1}">(ICON_FA_MOUNTAIN, Slice)
                : Str::Print<"{0}  {1}">(ICON_FA_MOUNTAIN, Terrain.Name);

            // Two terrains may answer to the same name, so the row is told apart by its slice instead.
            Toolkit::Composer::PushID(Slice);

            if (Toolkit::Composer::Selectable(Label, Slice == Selection))
            {
                GetContext().SetInteger(Session::kSelectionTerrain, Slice);
            }

            if (Toolkit::Composer::BeginPopupContextItem())
            {
                GetContext().SetInteger(Session::kSelectionTerrain, Slice);

                if (Toolkit::Composer::MenuItem("Delete"))
                {
                    mSplatset.RemoveTerrain(Slice);
                }

                Toolkit::Composer::Tooltip(
                    "Takes the terrain out of the palette. Its art stays in the array, since every painted "
                    "region names its art by slice, so whatever was painted with it goes on drawing."_Text);

                Toolkit::Composer::EndPopup();
            }

            Toolkit::Composer::PopID();
        }
        Toolkit::Composer::EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrain::DrawPreviewPanel()
    {
        const UInt16 Selection = static_cast<UInt16>(GetContext().GetInteger(Session::kSelectionTerrain, 0));

        if (Toolkit::Composer::BeginTabBar("##preview_tabs"))
        {
            ConstRetainer<Graphic::Material> Material = mSplatset.GetMaterial();

            for (const Texture Semantic : Enum::GetValues<Texture>())
            {
                ConstRetainer<Graphic::Image> Array = Material->GetImage(GetTextureID(Semantic));

                if (!Array || Selection >= Array->GetLayers())
                {
                    continue;
                }

                if (Toolkit::Composer::BeginTabItem(Enum::GetName(Semantic)))
                {
                    const ImVec2 Minimum = Plugin::ImGuiRenderer::GetLayeredTextureUV(Selection, ImVec2(0.0f, 0.0f));
                    const ImVec2 Maximum = Plugin::ImGuiRenderer::GetLayeredTextureUV(Selection, ImVec2(1.0f, 1.0f));

                    mPreview.Draw(
                        Plugin::ImGuiRenderer::GetLayeredTextureID(Array->GetHandle()),
                        Vector2(Array->GetWidth(), Array->GetHeight()),
                        Rect(Minimum.x, Minimum.y, Maximum.x, Maximum.y));

                    Toolkit::Composer::EndTabItem();
                }
            }
            Toolkit::Composer::EndTabBar();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrain::DrawStatusBar()
    {
        DrawBottomBar("##status_bar", [&](Real32)
        {
            const ConstSpan<Splatset::Terrain> Terrains = mSplatset.GetTerrains();
            const UInt16 Selection = static_cast<UInt16>(GetContext().GetInteger(Session::kSelectionTerrain, 0));

            const Str Label = (Selection < Terrains.GetSize())
                ? Str::Print<"{0}  slice {1} of {2}">(
                      Terrains[Selection].Name.IsEmpty() ? "(Unnamed)"_Text : Text(Terrains[Selection].Name),
                      Selection, Terrains.GetSize())
                : Str("No terrain selected");

            Toolkit::Composer::SetCursorPosX(
                (Toolkit::Composer::GetWindowWidth() - Toolkit::Composer::CalcTextSize(Label).x) * 0.5f);
            Toolkit::Composer::TextDisabled(Label);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrain::OnCommit()
    {
        mAssembler.Commit();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrain::DrawProperties()
    {
        const UInt16 Selection = static_cast<UInt16>(GetContext().GetInteger(Session::kSelectionTerrain, 0));

        if (Selection >= mSplatset.GetTerrains().GetSize())
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

        Toolkit::Composer::FieldInline("Feather");
        Toolkit::Composer::PushID("Feather");

        if (Real32 Feather = Terrain.Feather; Toolkit::Composer::InputFloat("##value", Feather, 0.01f, 0.05f, "%.3f"))
        {
            Terrain.Feather = Clamp(Feather, 0.0f, 1.0f);
        }
        Toolkit::Composer::PopID();

        Toolkit::Composer::Tooltip(
            "How wide a band the terrain's relief feathers over where it meets another. Small values cut "
            "sharply between the two, larger ones let them interleave."_Text);

        Toolkit::Composer::FieldInline("Tint");
        Toolkit::Composer::PushID("Tint");
        Toolkit::Composer::InputTintSmall("##value", Terrain.Tint);
        Toolkit::Composer::PopID();

        Toolkit::Composer::Section("Slots");

        DrawSlot("Albedo", Assembler::Slot::Albedo,
            "The art the terrain is coloured by."_Text);
        DrawSlot("Normal", Assembler::Slot::Normal,
            "Its surface normals, which a terrain may go without."_Text);
        DrawSlot("Height", Assembler::Slot::Height,
            "Its elevation, folded into the alpha of the colour, which decides what shows where two "
            "terrains meet."_Text);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrain::DrawSlot(Text Label, Assembler::Slot Slot, Text Hint)
    {
        const UInt16 Selection = static_cast<UInt16>(GetContext().GetInteger(Session::kSelectionTerrain, 0));
        const UInt64 Key       = Hash(Label);

        Ref<Toolkit::Browser> Browser = GetContext().GetBrowser();

        if (Str Picked; Browser.Consume(Key, Picked))
        {
            mAssembler.SetSource(Selection, Slot, Picked);
        }

        if (Selection >= mSplatset.GetTerrains().GetSize())
        {
            return;
        }

        // Read after the write, since naming an art moves the string the terrain holds it in.
        ConstRef<Splatset::Terrain> Terrain = mSplatset.GetTerrain(Selection);

        const Str Held(Slot == Assembler::Slot::Albedo ? Terrain.Albedo
                     : Slot == Assembler::Slot::Normal ? Terrain.Normal
                                                      : Terrain.Height);

        Toolkit::Composer::FieldInline(Label);
        Toolkit::Composer::PushID(Label);
        Toolkit::Composer::InputTextWithButton("##art", Held,
            [&](Text Path)
            {
                mAssembler.SetSource(Selection, Slot, Path);
            },
            ICON_FA_ELLIPSIS,
            [&]
            {
                Browser.Open(Key, ".png .jpg .tga .bmp .tex");
            },
            ImGuiInputTextFlags_EnterReturnsTrue);
        Toolkit::Composer::PopID();
        Toolkit::Composer::Tooltip(Hint);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrain::DrawSettings()
    {
        Toolkit::Composer::Section("General");

        static constexpr UInt16 kSizes[] = { 64, 128, 256, 512, 1024 };

        const UInt16 Current = mSplatset.GetResolution();

        Toolkit::Composer::FieldInline("Texture size");
        Toolkit::Composer::SetNextItemWidth(120.0f);

        if (Toolkit::Composer::BeginCombo("##resolution", String<16>::Print<"{0}">(Current)))
        {
            for (const UInt16 Size : kSizes)
            {
                if (Toolkit::Composer::Selectable(String<16>::Print<"{0}">(Size), Size == Current) && Size != Current)
                {
                    mSplatset.SetResolution(Size);
                    mAssembler.Rebuild();
                }
            }
            Toolkit::Composer::EndCombo();
        }

        Toolkit::Composer::Tooltip(
            "The extent every slice is baked at. Changing it bakes every terrain again from the art it "
            "names, and art of any size is scaled to fit."_Text);
    }
}