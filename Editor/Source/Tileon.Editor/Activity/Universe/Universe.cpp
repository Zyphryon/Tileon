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

#include "Universe.hpp"
#include "Tileon.Editor/Component/Descriptor.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Universe::Universe(Ref<Context> Context)
        : Activity  { Context, "Universe", true },
          mRegistry { Context.GetRegistry() },
          mSelector { Context.GetContent() },
          mAction   { Action::None }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Universe::OnDraw()
    {
        Toolkit::Composer::SetNextWindowSize(320.0f, 480.0f, ImGuiCond_FirstUseEver);

        if (Toolkit::Composer::Begin(GetTitle(), mVisible))
        {
            constexpr Real32 kFooterHeight = 40.0f;

            mAction  = Action::None;
            mSubject = Scene::Entity();

            Toolkit::Composer::BeginChild("##body", ImVec2(0, -kFooterHeight - Toolkit::Composer::GetStyle().ItemSpacing.y), ImGuiChildFlags_Borders);
            Toolkit::Composer::Section("Components");

            UInt32 Count = 0;

            for (const Scene::Entity Component : mRegistry.GetCatalog())
            {
                if (Component.Has(flecs::Singleton))
                {
                    DrawComponent(Component);

                    ++Count;
                }
            }

            if (Count == 0)
            {
                DrawEmptyPanel("The world holds no components");
            }
            Toolkit::Composer::EndChild();

            Toolkit::Composer::BeginChild("##footer", ImVec2(0, kFooterHeight));
            DrawCatalog();
            Toolkit::Composer::EndChild();

            Apply();
        }
        Toolkit::Composer::End();

        // The browser is modal, so it is drawn outside the window that hosts the fields which opened it.
        mSelector.Draw();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Universe::DrawComponent(Scene::Entity Component)
    {
        // A singleton's value lives on the component entity itself, so the world "holds" it only when that
        // entity carries its own component.
        const ConstPtr<Descriptor> Info    = Component.TryGet<const Descriptor>();

        const String<128> Label = String<128>::Print<"{0}  {1}##{2}">(Info->GetIcon(), Info->GetLabel(), Component.GetID());

        Workspace Workspace(GetContext(), mSelector);

        const Bool Present = Component.Has(Component);
        const Bool Open    = Toolkit::Composer::TreeNode(Label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);

        if (Toolkit::Composer::BeginPopupContextItem())
        {
            if (Present)
            {
                if (Toolkit::Composer::MenuItem("Remove"))
                {
                    mAction  = Action::Remove;
                    mSubject = Component;
                }
            }
            else if (Toolkit::Composer::MenuItem("Add"))
            {
                mAction  = Action::Add;
                mSubject = Component;
            }
            Toolkit::Composer::EndPopup();
        }

        if (Open)
        {
            if (!Present)
            {
                Toolkit::Composer::TextDisabled("Not present");
            }
            else if (Info->HasFields())
            {
                if (Info->Inspect(Workspace, Component, Component.TryGet(Component)))
                {
                    Component.Notify(Component);
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

    void Universe::DrawCatalog()
    {
        Toolkit::Composer::SetNextItemWidth(-1.0f);

        if (Toolkit::Composer::BeginCombo("##catalog", ICON_FA_PLUS "  Add Component", ImGuiComboFlags_HeightLarge))
        {
            Text   Group;
            UInt32 Count = 0;

            for (const Scene::Entity Component : mRegistry.GetCatalog())
            {
                if (!Component.Has(flecs::Singleton))
                {
                    continue;
                }

                const ConstPtr<Descriptor> Info = Component.TryGet<const Descriptor>();

                if (Group != Info->GetGroup())
                {
                    Group = Info->GetGroup();
                    Toolkit::Composer::Section(Group);
                }

                const Bool        Owned = Component.Has(Component);
                const String<128> Label = String<128>::Print<"{0}  {1}">(Info->GetIcon(), Info->GetLabel());

                Toolkit::Composer::BeginDisabled(Owned);

                if (Toolkit::Composer::Selectable(Label) && !Owned)
                {
                    mAction  = Action::Add;
                    mSubject = Component;
                }

                Toolkit::Composer::EndDisabled();

                ++Count;
            }

            if (Count == 0)
            {
                Toolkit::Composer::TextDisabled("No components are registered as singletons");
            }
            Toolkit::Composer::EndCombo();
        }
    }

    // -=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=-
    // -=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=--=-

    void Universe::DrawEmptyPanel(Text Message)
    {
        const ImVec2 Available = Toolkit::Composer::GetContentRegionAvail();
        const ImVec2 HintSize  = Toolkit::Composer::CalcTextSize(Message);

        Toolkit::Composer::SetCursorPosX((Available.x - HintSize.x) * 0.5f);
        Toolkit::Composer::SetCursorPosY((Available.y - HintSize.y) * 0.5f);
        Toolkit::Composer::TextDisabled(Message);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Universe::Apply()
    {
        if (mAction == Action::None)
        {
            return;
        }

        // A data-less tag has no storage to notify, so it can only ever be added or removed.
        const ConstPtr<Descriptor> Info = mSubject.TryGet<const Descriptor>();

        switch (mAction)
        {
        case Action::None:
            break;
        case Action::Add:
            mSubject.Add(mSubject);

            if (Info && Info->HasFields())
            {
                mSubject.Notify(mSubject);
            }
            break;
        case Action::Remove:
            mSubject.Remove(mSubject);
            break;
        }

        mAction  = Action::None;
        mSubject = Scene::Entity();
    }
}