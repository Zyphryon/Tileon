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

#include "Hierarchy.hpp"
#include "Tileon.World/Component/State/Lifecycle.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Hierarchy::Hierarchy(Ref<Context> Context)
        : Activity { Context, "Hierarchy", true }
    {
        Ref<Scene::Service> Scene = Context.GetScene();
        mRegions = Scene.CreateQuery<Scene::DSL::In<const Region>>("Editor::Hierarchy::Regions", Scene::Cache::Auto);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Hierarchy::OnDraw(Ref<UI::Composer> Composer)
    {
        Composer.SetNextWindowSize(320.0f, 480.0f, ImGuiCond_FirstUseEver);

        if (Composer.Begin(GetTitle(), mVisible))
        {
            Composer.BeginChild("##body", ImVec2(0, 0), ImGuiChildFlags_Borders);
            Composer.Section("Regions");

            UInt32 Count = 0;

            mRegions.Run<const Region>([&](Scene::Entity Actor, ConstRef<Region> Region)
            {
                DrawEntity(Composer, Actor);

                ++Count;
            });

            if (Count == 0)
            {
                DrawEmptyPanel(Composer, "No regions are currently loaded");
            }
            Composer.EndChild();

            // Pressing Delete removes the selected entity.
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && Composer.IsKeyPressed(ImGuiKey_Delete))
            {
                DeleteSelection();
            }
        }
        Composer.End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Hierarchy::DrawEntity(Ref<UI::Composer> Composer, Scene::Entity Actor)
    {
        Sequence<Scene::Entity> Children;   // TODO: Prevent Heap Allocation.

        Actor.Children([&Children](Scene::Entity Child)
        {
            Children.Append(Child);
        });

        const UInt64 Selection = GetContext().GetInteger("Selection.Entity", 0);

        ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (Children.IsEmpty())
        {
            Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        if (Actor.GetID() == Selection)
        {
            Flags |= ImGuiTreeNodeFlags_Selected;
        }

        // A prefab part is nested under another world entity; regions sit at the top and everything directly beneath
        // one is an independently-placed instance.
        const Text Icon = Actor.Has<Region>()
            ? ICON_FA_MAP
            : (Actor.GetParent(Scene::Hierarchy::Fixed).IsValid() ? ICON_FA_PUZZLE_PIECE : ICON_FA_CUBE);

        String<128> Label;

        if (const Text Display = Actor.GetAlias(); !Display.IsEmpty())
        {
            Label = String<128>::Print<"{0}  {1}###{2:016X}">(Icon, Display, Actor.GetID());
        }
        else if (const Scene::Entity Archetype = Actor.GetArchetype(); Archetype.IsValid())
        {
            Label = String<128>::Print<"{0}  {1}###{2:016X}">(Icon, Archetype.GetAlias(), Actor.GetID());
        }
        else
        {
            Label = String<128>::Print<"{0}  Entity {1:016X}###{2:016X}">(Icon, Actor.GetID(), Actor.GetID());
        }

        const Bool Open = Composer.TreeNode(Label, Flags);

        // Select on a plain click; a click that only toggled the expander must not steal the selection.
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            GetContext().SetInteger("Selection.Entity", Actor.GetID());
        }

        if (Composer.BeginPopupContextItem())
        {
            // A prefab part's identity is defined by its archetype, so it may not be renamed here; only
            // regions and independently-placed instances carry their own editable alias.
            if (Actor.GetParent(Scene::Hierarchy::Fixed).IsValid())
            {
                Composer.TextDisabled("Prefab part: name defined by its archetype");
            }
            else
            {
                Composer.InputText("##rename", Actor.GetAlias(), [Actor](Text Value)
                {
                    Actor.SetAlias(Value);
                });
            }
            Composer.EndPopup();
        }

        if (Open && !Children.IsEmpty())
        {
            for (const Scene::Entity Child : Children)
            {
                DrawEntity(Composer, Child);
            }
            Composer.TreePop();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Hierarchy::DeleteSelection()
    {
        Ref<Context>  Context = GetContext();
        Scene::Entity Actor   = Context.GetScene().GetEntity(Context.GetInteger("Selection.Entity", 0));

        // Regions own their own load/unload lifecycle and must never be deleted from the tree.
        if (!Actor.IsValid() || Actor.Has<Region>())
        {
            return;
        }

        // Resolve to the instance root so deleting a prefab part removes the whole placed instance, matching
        // the viewport's delete behaviour and never leaving a structurally-locked part orphaned.
        Actor = Scene::Entity::ResolveRecursively(Actor, Scene::Hierarchy::Fixed);

        if (const Scene::Entity Region = Actor.GetParent(); Region.IsValid())
        {
            Region.Add<Persist>();
        }

        Actor.Add<Dispose>();
        Context.SetInteger("Selection.Entity", 0);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Hierarchy::DrawEmptyPanel(Ref<UI::Composer> Composer, Text Message)
    {
        const ImVec2 Available = Composer.GetContentRegionAvail();
        const ImVec2 HintSize  = Composer.CalcTextSize(Message);

        Composer.SetCursorPosX((Available.x - HintSize.x) * 0.5f);
        Composer.SetCursorPosY((Available.y - HintSize.y) * 0.5f);
        Composer.TextDisabled(Message);
    }
}
