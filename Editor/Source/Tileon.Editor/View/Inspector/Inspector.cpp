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

#include "Inspector.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Inspector::Inspector(Ref<Context> Context)
        : Activity   { Context, "Inspector", true },
          mAssembler { Context }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Inspector::OnDraw(Ref<UI::Composer> Composer)
    {
        Composer.SetNextWindowSize(300, 500, ImGuiCond_FirstUseEver);

        if (Composer.Begin(GetTitle(), mVisible))
        {
            constexpr Real32 kFooterHeight = 40.0f;

            // The selection can be destroyed from elsewhere while it is still being pointed at.
            const Scene::Entity Actor = GetContext().GetScene().GetEntity(GetContext().GetInteger("Selection.Entity"));
            const Bool          Alive = Actor.IsValid() && Actor.IsAlive();

            if (Alive)
            {
                DrawHeader(Composer, Actor);
                Composer.Separator();
            }

            Composer.BeginChild("##body", ImVec2(0, -kFooterHeight - ImGui::GetStyle().ItemSpacing.y), ImGuiChildFlags_Borders);

            if (Alive)
            {
                DrawBody(Composer, Actor);
            }
            else
            {
                DrawEmptyPanel(Composer, "No entity selected");
            }
            Composer.EndChild();

            Composer.BeginChild("##footer", ImVec2(0, kFooterHeight));

            if (Alive)
            {
                DrawFooter(Composer, Actor);
            }
            Composer.EndChild();
        }
        Composer.End();

        // The browser is modal, so it is drawn outside the window that hosts the fields which opened it.
        mAssembler.DrawSelector(Composer);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Inspector::DrawHeader(Ref<UI::Composer> Composer, Scene::Entity Actor)
    {
        Composer.Field("ID");
        Composer.Label("{0:016X}", Actor.GetID());
        Composer.Spacing();

        Composer.Field("Name");
        Composer.SetNextItemWidth(-1.0f);
        Composer.Label(Actor.GetName());
        Composer.Spacing();

        Composer.Field("Alias");
        Composer.SetNextItemWidth(-1.0f);
        Composer.InputText("##entity_alias", Actor.GetAlias(), [&](Text Value)
        {
            Actor.SetAlias(Value);
        });
        Composer.Spacing();

        Composer.Field("Archetype");

        if (const Scene::Entity Archetype = Actor.GetArchetype(); Archetype.IsValid())
        {
            Composer.Label("{0}", Archetype.GetAlias());
        }
        else
        {
            Composer.TextDisabled("None");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Inspector::DrawBody(Ref<UI::Composer> Composer, Scene::Entity Actor)
    {
        // Make the override-only nature explicit: a part's structure belongs to its archetype, not this instance.
        if (Actor.GetParent(Scene::Hierarchy::Fixed).IsValid())
        {
            Composer.TextDisabled(ICON_FA_PUZZLE_PIECE "  Prefab part: structure is defined by its archetype");
            Composer.Spacing();
        }

        Composer.Section("Components");

        mAssembler.Draw(Composer, Actor);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Inspector::DrawFooter(Ref<UI::Composer> Composer, Scene::Entity Actor)
    {
        if (Bool Awake = Actor.IsAwake(); Composer.Checkbox("Awake", Awake))
        {
            Awake ? Actor.Awake() : Actor.Sleep();
        }

        Composer.SameLine();

        // A prefab part is owned by its archetype and a region owns its own load/unload lifecycle, so neither may
        // be destroyed from here; only an independently-placed instance can be removed.
        const Bool Destroyable = !Actor.GetParent(Scene::Hierarchy::Fixed).IsValid() && !Actor.Has<Region>();

        Composer.BeginDisabled(!Destroyable);

        if (Composer.Button(ICON_FA_TRASH "  Destroy") && Destroyable)
        {
            Actor.Add<Dispose>();

            GetContext().SetInteger("Selection.Entity", 0);
        }

        Composer.EndDisabled();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Inspector::DrawEmptyPanel(Ref<UI::Composer> Composer, Text Message)
    {
        const ImVec2 Available = Composer.GetContentRegionAvail();
        const ImVec2 HintSize  = Composer.CalcTextSize(Message);

        Composer.SetCursorPosX((Available.x - HintSize.x) * 0.5f);
        Composer.SetCursorPosY((Available.y - HintSize.y) * 0.5f);
        Composer.TextDisabled(Message);
    }
}
