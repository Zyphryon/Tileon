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

#include "ComponentList.hpp"
#include "ComponentType.hpp"
#include "Tileon.Editor/Utility.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ComponentList::ComponentList(Ref<Context> Context)
        : mContext { Context },
          mCatalog { Context.GetCatalog() },
          mBrowser { Context.GetContent() },
          mAction  { Action::None }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void ComponentList::Draw(Scene::Entity Actor)
    {
        if (!Actor.IsValid())
        {
            return;
        }

        mAction  = Action::None;
        mSubject = Scene::Entity();

        // Draw the components the entity owns.
        Actor.Each([&](Scene::Entity Component)
        {
            if (!ecs_id_is_pair(Component.GetID()) && IsAuthorable(Actor, Component))
            {
                DrawComponent(Actor, Component, false);
            }
        });

        // Draw the components still supplied by the archetype.
        if (const Scene::Entity Archetype = Actor.GetArchetype(); Archetype.IsValid())
        {
            Archetype.Each([&](Scene::Entity Component)
            {
                if (!ecs_id_is_pair(Component.GetID()) && IsAuthorable(Actor, Component) && !Actor.Owns(Component.GetID()))
                {
                    DrawComponent(Actor, Component, true);
                }
            });
        }

        Toolkit::Composer::Spacing();
        DrawCatalog(Actor);

        Apply(Actor);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool ComponentList::IsAuthorable(Scene::Entity Actor, Scene::Entity Component) const
    {
        const ConstPtr<ComponentType> Info = Component.TryGet<const ComponentType>();

        return Info && Info->IsAuthorableOn(Actor.IsArchetype());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void ComponentList::DrawComponent(Scene::Entity Actor, Scene::Entity Component, Bool Inherited)
    {
        const ConstPtr<ComponentType> Info  = Component.TryGet<const ComponentType>();
        const String<128>          Label = String<128>::Print<"{0}  {1}##{2}">(Info->GetIcon(), Info->GetLabel(), Component.GetID());

        // The browser belongs to this assembler, so two views can each have one open on their own subject.
        Workspace Workspace(mContext, mBrowser);

        const Bool Open = Toolkit::Composer::TreeNode(Label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);

        if (Toolkit::Composer::BeginPopupContextItem())
        {
            if (Inherited)
            {
                if (Toolkit::Composer::MenuItem("Override"))
                {
                    mAction  = Action::Override;
                    mSubject = Component;
                }
            }
            else
            {
                const Scene::Entity Archetype = Actor.GetArchetype();
                const Bool Revert = Archetype.IsValid() && Archetype.Has(Component);

                if (Toolkit::Composer::MenuItem(Revert ? "Revert to Archetype"_Text : "Remove"_Text))
                {
                    mAction  = Action::Remove;
                    mSubject = Component;
                }
            }
            Toolkit::Composer::EndPopup();
        }

        if (Open)
        {
            if (Inherited)
            {
                const Scene::Entity Archetype = Actor.GetArchetype();

                Toolkit::Composer::TextDisabled("Inherited from {0}", Archetype.GetAlias());

                // The values on show belong to the archetype, so it is the archetype an inspector would be editing.
                if (Info->HasFields())
                {
                    Toolkit::Composer::BeginDisabled();
                    Info->Inspect(Workspace, Archetype, Archetype.TryGet(Component));
                    Toolkit::Composer::EndDisabled();
                }

                if (Toolkit::Composer::SmallButton("Override"))
                {
                    mAction  = Action::Override;
                    mSubject = Component;
                }
            }
            else if (Info->HasFields())
            {
                if (Info->Inspect(Workspace, Actor, Actor.TryGet(Component)))
                {
                    Actor.Notify(Component);

                    Touch(Actor);
                }
            }
            else
            {
                Toolkit::Composer::TextDisabled("No properties");
            }
            Toolkit::Composer::TreePop();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void ComponentList::DrawCatalog(Scene::Entity Actor)
    {
        Toolkit::Composer::SetNextItemWidth(-1.0f);

        if (Toolkit::Composer::BeginCombo("##catalog", ICON_FA_PLUS "  Add Component", ImGuiComboFlags_HeightLarge))
        {
            Text Group;

            for (const Scene::Entity Component : mCatalog.GetComponents())
            {
                // A singleton's data lives on the component entity itself, so attaching it to an actor would be
                // meaningless. It still carries a descriptor, so a world-level panel can draw it.
                if (!IsAuthorable(Actor, Component) || Component.Has(flecs::Singleton))
                {
                    continue;
                }

                const ConstPtr<ComponentType> Info = Component.TryGet<const ComponentType>();

                if (Group != Info->GetGroup())
                {
                    Group = Info->GetGroup();
                    Toolkit::Composer::Section(Group);
                }

                // Anything the entity already has, by ownership or inheritance, is shown but not offered.
                const Bool        Owned = Actor.Has(Component);
                const String<128> Label = String<128>::Print<"{0}  {1}">(Info->GetIcon(), Info->GetLabel());

                Toolkit::Composer::BeginDisabled(Owned);

                if (Toolkit::Composer::Selectable(Label) && !Owned)
                {
                    mAction  = Action::Add;
                    mSubject = Component;
                }

                Toolkit::Composer::EndDisabled();
            }
            Toolkit::Composer::EndCombo();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void ComponentList::Apply(Scene::Entity Actor)
    {
        if (mAction == Action::None)
        {
            return;
        }

        // A data-less tag has no storage to ensure or notify, so it can only ever be added or removed.
        const ConstPtr<ComponentType> Info = mSubject.TryGet<const ComponentType>();

        switch (mAction)
        {
        case Action::None:
            break;
        case Action::Add:
            Actor.Add(mSubject);

            if (Info && Info->HasFields())
            {
                Actor.Notify(mSubject);
            }
            break;
        case Action::Remove:
            Actor.Remove(mSubject);
            break;
        case Action::Override:
            if (Info && Info->HasFields())
            {
                Actor.Ensure(mSubject);
                Actor.Notify(mSubject);
            }
            else
            {
                Actor.Add(mSubject);
            }
            break;
        }

        Touch(Actor);

        mAction  = Action::None;
        mSubject = Scene::Entity();
    }
}