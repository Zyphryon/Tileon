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

#include "Assembler.hpp"
#include "Descriptor.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Assembler::Assembler(Ref<Context> Context)
        : mContext  { Context },
          mRegistry { Context.GetRegistry() },
          mAction   { Action::None }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assembler::Draw(Ref<UI::Composer> Composer, Scene::Entity Actor)
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
            if (!Component.IsPair() && IsAuthorable(Actor, Component))
            {
                DrawComponent(Composer, Actor, Component, false);
            }
        });

        // Draw the components still supplied by the archetype.
        if (const Scene::Entity Archetype = Actor.GetArchetype(); Archetype.IsValid())
        {
            Archetype.Each([&](Scene::Entity Component)
            {
                if (!Component.IsPair() && IsAuthorable(Actor, Component) && !Actor.GetHandle().owns(Component.GetID()))
                {
                    DrawComponent(Composer, Actor, Component, true);
                }
            });
        }

        Composer.Spacing();
        DrawCatalog(Composer, Actor);

        Apply(Actor);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Assembler::IsAuthorable(Scene::Entity Actor, Scene::Entity Component) const
    {
        const ConstPtr<Descriptor> Info = Component.TryGet<const Descriptor>();

        return Info && Info->IsAuthorableOn(Actor.IsArchetype());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assembler::DrawComponent(Ref<UI::Composer> Composer, Scene::Entity Actor, Scene::Entity Component, Bool Inherited)
    {
        const ConstPtr<Descriptor> Info  = Component.TryGet<const Descriptor>();
        const String<128>          Label = String<128>::Print<"{0}  {1}##{2}">(Info->GetIcon(), Info->GetLabel(), Component.GetID());

        const Bool Open = Composer.TreeNode(Label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);

        if (Composer.BeginPopupContextItem())
        {
            if (Inherited)
            {
                if (Composer.MenuItem("Override"))
                {
                    mAction  = Action::Override;
                    mSubject = Component;
                }
            }
            else
            {
                const Scene::Entity Archetype = Actor.GetArchetype();
                const Bool Revert = Archetype.IsValid() && Archetype.Has(Component);

                if (Composer.MenuItem(Revert ? "Revert to Archetype"_Text : "Remove"_Text))
                {
                    mAction  = Action::Remove;
                    mSubject = Component;
                }
            }
            Composer.EndPopup();
        }

        if (Open)
        {
            if (Inherited)
            {
                const Scene::Entity Archetype = Actor.GetArchetype();

                Composer.TextDisabled("Inherited from {0}", Archetype.GetAlias());

                // The values on show belong to the archetype, so it is the archetype an inspector would be editing.
                Composer.BeginDisabled();
                Info->Inspect(Composer, mContext, Archetype, Archetype.TryGet(Component));
                Composer.EndDisabled();

                if (Composer.SmallButton("Override"))
                {
                    mAction  = Action::Override;
                    mSubject = Component;
                }
            }
            else if (Info->HasFields())
            {
                if (Info->Inspect(Composer, mContext, Actor, Actor.TryGet(Component)))
                {
                    Actor.Notify(Component);
                }
            }
            else
            {
                Composer.TextDisabled("No properties");
            }
            Composer.TreePop();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assembler::DrawCatalog(Ref<UI::Composer> Composer, Scene::Entity Actor)
    {
        Composer.SetNextItemWidth(-1.0f);

        if (Composer.BeginCombo("##catalog", ICON_FA_PLUS "  Add Component", ImGuiComboFlags_HeightLarge))
        {
            Text Group;

            for (const Scene::Entity Component : mRegistry.GetCatalog())
            {
                // A singleton's data lives on the component entity itself, so attaching it to an actor would be
                // meaningless. It still carries a descriptor, so a world-level panel can draw it.
                if (!IsAuthorable(Actor, Component) || Component.Has(flecs::Singleton))
                {
                    continue;
                }

                const ConstPtr<Descriptor> Info = Component.TryGet<const Descriptor>();

                if (Group != Info->GetGroup())
                {
                    Group = Info->GetGroup();
                    Composer.Section(Group);
                }

                // Anything the entity already has, by ownership or inheritance, is shown but not offered.
                const Bool        Owned = Actor.Has(Component);
                const String<128> Label = String<128>::Print<"{0}  {1}">(Info->GetIcon(), Info->GetLabel());

                Composer.BeginDisabled(Owned);

                if (Composer.Selectable(Label) && !Owned)
                {
                    mAction  = Action::Add;
                    mSubject = Component;
                }

                Composer.EndDisabled();
            }
            Composer.EndCombo();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assembler::Apply(Scene::Entity Actor)
    {
        if (mAction == Action::None)
        {
            return;
        }

        // A data-less tag has no storage to ensure or notify, so it can only ever be added or removed.
        const ConstPtr<Descriptor> Info      = mSubject.TryGet<const Descriptor>();
        const Bool                 HasFields = Info && Info->HasFields();

        switch (mAction)
        {
        case Action::None:
            break;
        case Action::Add:
            Actor.Add(mSubject);

            if (HasFields)
            {
                Actor.Notify(mSubject);
            }
            break;
        case Action::Remove:
            Actor.Remove(mSubject);
            break;
        case Action::Override:
            if (HasFields)
            {
                // Ensuring an inherited component copies the archetype's value onto the entity.
                Actor.Ensure(mSubject);
                Actor.Notify(mSubject);
            }
            else
            {
                Actor.Add(mSubject);
            }
            break;
        }

        mAction  = Action::None;
        mSubject = Scene::Entity();
    }
}
