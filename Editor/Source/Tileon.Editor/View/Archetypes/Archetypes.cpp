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

#include "Zyphryon.Base/Primitive.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Archetypes::Archetypes(Ref<Context> Context)
        : Activity    { Context, "Archetypes" },
          mRepository { Context.GetRepository() }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::OnDraw(Ref<UI::Composer> Composer)
    {
        Composer.SetNextWindowSize(760.0f, 520.0f, ImGuiCond_FirstUseEver);
        Composer.SetNextWindowSizeConstraints(560.0f, 360.0f, 1400.0f, 1200.0f);

        if (Composer.Begin(GetTitle(), mVisible))
        {
            const Real32 BodyHeight = -(Composer.GetFrameHeightWithSpacing() + 8.0f);

            Composer.BeginChild("##list_panel", ImVec2(260.0f, BodyHeight), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders);
            DrawListPanel(Composer);
            Composer.EndChild();

            if (mSelection.IsValid())
            {
                Composer.SameLine();
                Composer.BeginChild("##details_panel", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_Borders);
                DrawDetailsPanel(Composer);
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
                Archetype.SetDisplayName(Format("Archetype.{}", Archetype.GetID() - Scene::kMinRangeArchetypes));

                mSelection = Archetype;
            }
        }

        Composer.Separator();
        Composer.BeginChild("##list_scroll");

        // Build a temporary collection of archetypes and sort them by their ID.
        Vector<Scene::Entity> Collection;
        mRepository.ForEachArchetype([&](ConstRef<Scene::Entity> Archetype)
        {
            Collection.push_back(Archetype);
        });

        std::ranges::sort(Collection, [](Scene::Entity A, Scene::Entity B)
        {
            return A.GetID() < B.GetID();
        });

        for (Scene::Entity Archetype : Collection)
        {
            const Bool Selected = (mSelection == Archetype);

            Composer.Selectable(Archetype.GetDisplayName(), Selected);

            if (Composer.IsItemClicked())
            {
                mSelection = Archetype;
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
        }

        Composer.EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawDetailsPanel(Ref<UI::Composer> Composer)
    {
        Composer.Section("Identity");

        Composer.Field("ID");
        Composer.Label("{:016X}", mSelection.GetID());
        Composer.Spacing();

        Composer.Field("Name");
        Composer.SetNextItemWidth(-1.0f);
        Composer.InputText("##archetype_name", mSelection.GetName(), [&](ConstStr8 Value)
        {
            mSelection.SetName(Value);
        });
        Composer.Spacing();

        Composer.Field("Display");
        Composer.SetNextItemWidth(-1.0f);
        Composer.InputText("##archetype_display", mSelection.GetDisplayName(), [&](ConstStr8 Value)
        {
            mSelection.SetDisplayName(Value);
        });
        Composer.Spacing();

        Composer.Section("Components");

        // TODO: Components
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
            const ConstStr8 Label = mSelection.GetDisplayName();

            Composer.SetCursorPosX((Composer.GetWindowWidth() - Composer.CalcTextSize(Label).x) * 0.5f);
            Composer.TextDisabled(Label);
        }
        else
        {
            constexpr ConstStr8 Hint = "No archetype selected";

            Composer.SetCursorPosX((Composer.GetWindowWidth() - Composer.CalcTextSize(Hint).x) * 0.5f);
            Composer.TextDisabled(Hint);
        }

        Composer.EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawEmptyPanel(Ref<UI::Composer> Composer, ConstStr8 Message)
    {
        constexpr ConstStr8 kIcon = "?";

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

