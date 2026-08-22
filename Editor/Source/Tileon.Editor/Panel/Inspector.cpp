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
#include "Tileon.Editor/Utility.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Inspector::Inspector(Ref<Context> Context)
        : Panel       { Context, "Inspector", true },
          mComponents { Context },
          mEditing    { false }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Inspector::OnDraw()
    {
        Toolkit::Composer::SetNextWindowSize(300, 500, ImGuiCond_FirstUseEver);

        if (Toolkit::Composer::Begin(GetTitle(), mVisible))
        {
            constexpr Real32 kFooterHeight = 40.0f;

            // The selection can be destroyed from elsewhere while it is still being pointed at.
            const Scene::Entity Actor = GetContext().GetScene().GetEntity(GetContext().GetInteger(Session::kSelectionEntity));
            const Bool          Alive = Actor.IsValid() && Actor.IsAlive();

            if (Alive)
            {
                DrawHeader(Actor);
                Toolkit::Composer::Separator();
            }

            Toolkit::Composer::BeginChild("##body", ImVec2(0, -kFooterHeight - Toolkit::Composer::GetStyle().ItemSpacing.y), ImGuiChildFlags_Borders);

            if (Alive)
            {
                DrawBody(Actor);
            }
            else
            {
                DrawEmptyPanel("No entity selected");
            }
            Toolkit::Composer::EndChild();

            TrackEdit(Alive ? Actor : Scene::Entity());

            Toolkit::Composer::BeginChild("##footer", ImVec2(0, kFooterHeight));

            if (Alive)
            {
                DrawFooter(Actor);
            }
            Toolkit::Composer::EndChild();
        }
        Toolkit::Composer::End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Inspector::DrawHeader(Scene::Entity Actor)
    {
        Toolkit::Composer::Field("ID");
        Toolkit::Composer::Label("{0:016X}", Actor.GetID());
        Toolkit::Composer::Spacing();

        Toolkit::Composer::Field("Name");
        Toolkit::Composer::SetNextItemWidth(-1.0f);
        Toolkit::Composer::Label(Actor.GetName());
        Toolkit::Composer::Spacing();

        Toolkit::Composer::Field("Alias");
        Toolkit::Composer::SetNextItemWidth(-1.0f);
        Toolkit::Composer::InputText("##entity_alias", Actor.GetAlias(), [&](Text Value)
        {
            Actor.SetAlias(Value);

            Touch(Actor);
        });
        Toolkit::Composer::Spacing();

        Toolkit::Composer::Field("Archetype");

        if (const Scene::Entity Archetype = Actor.GetArchetype(); Archetype.IsValid())
        {
            // The name is the way into what it names, the same jump the palette offers on right click.
            if (Toolkit::Composer::Selectable(String<128>::Print<"{0}###archetype">(Archetype.GetAlias())))
            {
                GetContext().SetString(Session::kNavigatePanel, "Archetypes");
                GetContext().SetInteger(Session::kSelectionArchetype, Archetype.GetID());
                GetContext().SetInteger(Session::kSelectionArchetypeTarget, Archetype.GetID());
            }

            Toolkit::Composer::Tooltip("Open this archetype in the Archetypes panel");
        }
        else
        {
            Toolkit::Composer::TextDisabled("None");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Inspector::DrawBody(Scene::Entity Actor)
    {
        // Make the override-only nature explicit: a part's structure belongs to its archetype, not this instance.
        if (Actor.GetParent(Scene::Hierarchy::Fixed).IsValid())
        {
            Toolkit::Composer::TextDisabled(ICON_FA_PUZZLE_PIECE "  Prefab part: structure is defined by its archetype");
            Toolkit::Composer::Spacing();
        }

        Toolkit::Composer::Section("Components");

        mComponents.Draw(Actor);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Inspector::TrackEdit(Scene::Entity Actor)
    {
        const Bool Editing = Actor.IsValid()
            && Toolkit::Composer::IsAnyItemActive()
            && Toolkit::Composer::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        if (Editing != mEditing)
        {
            Ref<History> History = GetContext().GetHistory();

            if (Editing)
            {
                History.Open("Edit");
                History.CaptureEntity(Actor);
            }
            else
            {
                History.Close();
            }
            mEditing = Editing;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Inspector::DrawFooter(Scene::Entity Actor)
    {
        if (Bool Awake = Actor.IsAwake(); Toolkit::Composer::Checkbox("Awake", Awake))
        {
            Awake ? Actor.Awake() : Actor.Sleep();
        }

        Toolkit::Composer::SameLine();

        // A prefab part is owned by its archetype and a region owns its own load/unload lifecycle, so neither may
        // be destroyed from here; only an independently-placed instance can be removed.
        const Bool Destroyable = !Actor.GetParent(Scene::Hierarchy::Fixed).IsValid() && !Actor.Has<Region>();

        Toolkit::Composer::BeginDisabled(!Destroyable);

        if (Toolkit::Composer::Button(ICON_FA_TRASH "  Destroy") && Destroyable)
        {
            Ref<History> History = GetContext().GetHistory();

            History.Open("Destroy");
            History.DiscardEntity(Actor);
            History.Close();

            Actor.Add<Dispose>();

            GetContext().SetInteger(Session::kSelectionEntity, 0);
        }

        Toolkit::Composer::EndDisabled();
    }
}