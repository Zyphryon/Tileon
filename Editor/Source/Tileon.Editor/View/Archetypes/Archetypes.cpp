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

#include "Archetypes.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Archetypes::Archetypes(Ref<Context> Context)
        : Activity    { Context, "Archetypes" },
          mRepository { Context.GetRepository() },
          mAssembler  { Context }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::OnDraw(Ref<UI::Composer> Composer)
    {
        Composer.SetNextWindowSize(1100.0f, 620.0f, ImGuiCond_FirstUseEver);
        Composer.SetNextWindowSizeConstraints(560.0f, 360.0f, 1800.0f, 1400.0f);

        if (Composer.Begin(GetTitle(), mVisible))
        {
            const Real32 BodyHeight = -(Composer.GetFrameHeightWithSpacing() + 8.0f);

            Composer.BeginChild("##list_panel", ImVec2(260.0f, BodyHeight), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders);
            DrawListPanel(Composer);
            Composer.EndChild();

            if (mSelection.IsValid())
            {
                Composer.SameLine();
                Composer.BeginChild("##details_panel", ImVec2(380.0f, BodyHeight), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders);
                DrawDetailsPanel(Composer);
                Composer.EndChild();

                Composer.SameLine();
                Composer.BeginChild("##preview_panel", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_Borders);
                DrawPreviewPanel(Composer);
                Composer.EndChild();
            }
            else
            {
                Composer.SameLine();
                Composer.BeginChild("##empty_panel", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_Borders);
                DrawEmptyPanel(Composer, "No archetype selected");
                Composer.EndChild();
            }

            DrawBottomBar(Composer);
        }
        Composer.End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawListPanel(Ref<UI::Composer> Composer)
    {
        const Bool WasPlusClicked = Composer.Button("+", -1.0f);

        if (WasPlusClicked)
        {
            if (const Scene::Entity Archetype = mRepository.CreateArchetype(); Archetype.IsValid())
            {
                Archetype.SetAlias(Str32::Print<"Archetype.{0}">(Archetype.GetID() - Scene::kMinRangeArchetypes));

                mSelection = Archetype;
            }
        }

        Composer.Separator();
        Composer.BeginChild("##list_scroll");

        mRepository.ForEachArchetype([&](Scene::Entity Archetype)
        {
            const Bool Selected = (mSelection == Archetype);

            Composer.Selectable(Archetype.GetAlias(), Selected);

            if (Composer.IsItemClicked() && mSelection != Archetype)
            {
                mSelection = Archetype;

                mPreviewer.Reset();
            }

            if (Selected && WasPlusClicked)
            {
                Composer.SetScrollHereY(0.5f);
            }

            if (Composer.BeginPopupContextItem())
            {
                if (Composer.MenuItem("Delete"))
                {
                    mRepository.DeleteArchetype(Archetype);
                }
                Composer.EndPopup();
            }
        });

        Composer.EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawDetailsPanel(Ref<UI::Composer> Composer)
    {
        Composer.Section("Identity");

        Composer.Field("ID");
        Composer.Label("{0:016X}", mSelection.GetID());
        Composer.Spacing();

        Composer.Field("Name");
        Composer.SetNextItemWidth(-1.0f);
        Composer.InputText("##archetype_name", mSelection.GetName(), [&](Text Value)
        {
            mSelection.SetName(Value);
        });
        Composer.Spacing();

        Composer.Field("Alias");
        Composer.SetNextItemWidth(-1.0f);
        Composer.InputText("##archetype_display", mSelection.GetAlias(), [&](Text Value)
        {
            mSelection.SetAlias(Value);
        });
        Composer.Spacing();

        Composer.Section("Components");

        mAssembler.Draw(Composer, mSelection);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawPreviewPanel(Ref<UI::Composer> Composer)
    {
        const ConstPtr<Appearance> Visual = mSelection.TryGet<const Appearance>();

        if (!Visual)
        {
            DrawEmptyPanel(Composer, mSelection.Has<Typeface>()
                ? "Text preview unavailable"_Text
                : "This archetype has nothing to preview"_Text);
            return;
        }

        ConstRetainer<Graphic::Material> Material = Visual->GetMaterial();

        if (!Material)
        {
            DrawEmptyPanel(Composer, "No material assigned to this archetype");
            return;
        }

        if (!Material->HasCompleted())
        {
            DrawEmptyPanel(Composer, Material->HasFailed() ? "Material failed to load"_Text : "Loading material..."_Text);
            return;
        }

        if (Composer.BeginTabBar("##preview_tabs"))
        {
            if (ConstRetainer<Graphic::Image> Albedo = Material->GetImage(Graphic::TextureSlot::Albedo))
            {
                if (Composer.BeginTabItem("Preview"))
                {
                    const Rect    Source = Visual->GetSource();
                    const Vector2 Size(Source.GetWidth() * Albedo->GetWidth(), Source.GetHeight() * Albedo->GetHeight());

                    const ConstPtr<IntColor8> Tint = mSelection.TryGet<const IntColor8>();

                    mPreviewer.Draw(Composer, Albedo->GetHandle(), Size, Source, Tint ? Color::FromColor8(* Tint) : Color::White());

                    Composer.EndTabItem();
                }
            }

            for (const Graphic::TextureSlot Semantic : Enum::GetValues<Graphic::TextureSlot>())
            {
                if (ConstRetainer<Graphic::Image> Texture = Material->GetImage(Semantic))
                {
                    if (Composer.BeginTabItem(Enum::GetName(Semantic)))
                    {
                        const Vector2 Size(Texture->GetWidth(), Texture->GetHeight());
                        mPreviewer.Draw(Composer, Texture->GetHandle(), Size, Rect::One());

                        Composer.EndTabItem();
                    }
                }
            }

            Composer.EndTabBar();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawBottomBar(Ref<UI::Composer> Composer)
    {
        const Real32 BarHeight = Composer.GetFrameHeightWithSpacing() + 4.0f;

        Composer.PushStyleColor(ImGuiCol_ChildBg, Composer.GetStyleColorVec4(ImGuiCol_MenuBarBg));
        Composer.BeginChild("##status_bar", ImVec2(0.0f, BarHeight), ImGuiChildFlags_None);
        Composer.PopStyleColor();

        // Vertically center text inside the bar.
        const Real32 PadY = (BarHeight - Composer.GetTextLineHeight()) * 0.5f - Composer.GetStyle().ItemSpacing.y * 0.5f;
        Composer.SetCursorPosY(PadY);

        if (mSelection.IsValid())
        {
            const Text Label = mSelection.GetAlias();

            Composer.SetCursorPosX((Composer.GetWindowWidth() - Composer.CalcTextSize(Label).x) * 0.5f);
            Composer.TextDisabled(Label);
        }
        else
        {
            constexpr Text Hint = "No archetype selected";

            Composer.SetCursorPosX((Composer.GetWindowWidth() - Composer.CalcTextSize(Hint).x) * 0.5f);
            Composer.TextDisabled(Hint);
        }

        Composer.EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawEmptyPanel(Ref<UI::Composer> Composer, Text Message)
    {
        constexpr Text kIcon = "?";

        const ImVec2 Available = Composer.GetContentRegionAvail();
        const ImVec2 IconSize  = Composer.CalcTextSize(kIcon);
        const ImVec2 HintSize  = Composer.CalcTextSize(Message);

        Composer.SetCursorPosX((Available.x - IconSize.x) * 0.5f);
        Composer.SetCursorPosY((Available.y - IconSize.y + 8.0f + HintSize.y) * 0.5f);
        Composer.TextDisabled(kIcon);

        Composer.SetCursorPosX((Available.x - HintSize.x) * 0.5f);
        Composer.TextDisabled(Message);
    }
}

