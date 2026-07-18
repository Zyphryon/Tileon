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

        String<128> Label;

        if (const Text Display = Actor.GetAlias(); !Display.IsEmpty())
        {
            if (Actor.Has<Region>())
            {
                Label = String<128>::Print<"{0}  {1}###{2:016X}">(ICON_FA_MAP, Display, Actor.GetID());
            }
            else
            {
                Label = String<128>::Print<"{0}  {1}###{2:016X}">(ICON_FA_CUBE, Display, Actor.GetID());
            }
        }
        else if (const Scene::Entity Archetype = Actor.GetArchetype(); Archetype.IsValid())
        {
            Label = String<128>::Print<"{0}  {1}###{2:016X}">(ICON_FA_CUBE, Archetype.GetAlias(), Actor.GetID());
        }
        else
        {
            Label = String<128>::Print<"{0}  Entity {1:016X}###{2:016X}">(ICON_FA_CUBE, Actor.GetID(), Actor.GetID());
        }

        const Bool Open = Composer.TreeNode(Label, Flags);

        // Select on a plain click; a click that only toggled the expander must not steal the selection.
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            GetContext().SetInteger("Selection.Entity", Actor.GetID());
        }

        if (Composer.BeginPopupContextItem())
        {
            Composer.InputText("##rename", Actor.GetAlias(), [Actor](Text Value)
            {
                Actor.SetAlias(Value);
            });
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

    void Hierarchy::DrawEmptyPanel(Ref<UI::Composer> Composer, Text Message)
    {
        const ImVec2 Available = Composer.GetContentRegionAvail();
        const ImVec2 HintSize  = Composer.CalcTextSize(Message);

        Composer.SetCursorPosX((Available.x - HintSize.x) * 0.5f);
        Composer.SetCursorPosY((Available.y - HintSize.y) * 0.5f);
        Composer.TextDisabled(Message);
    }
}
