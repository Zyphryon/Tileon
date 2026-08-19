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

#include "Environment.hpp"
#include "Tileon.Editor/Inspect/ComponentType.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Environment::Environment(Ref<Context> Context)
        : Panel    { Context, "Environment", true },
          mCatalog { Context.GetCatalog() },
          mAction  { Action::None },
          mEditing { false }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Environment::OnDraw()
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

            for (const Scene::Entity Component : mCatalog.GetComponents())
            {
                if (Component.Has(EcsSingleton))
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

            TrackEdit();

            Toolkit::Composer::BeginChild("##footer", ImVec2(0, kFooterHeight));
            DrawCatalog();
            Toolkit::Composer::EndChild();

            Apply();
        }
        Toolkit::Composer::End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Environment::DrawComponent(Scene::Entity Component)
    {
        // A singleton's value lives on the component entity itself, so the world "holds" it only when that
        // entity carries its own component.
        const ConstPtr<ComponentType> Info = Component.TryGet<const ComponentType>();

        const String<128> Label = String<128>::Print<"{0}  {1}##{2}">(Info->GetIcon(), Info->GetLabel(), Component.GetID());

        Workspace Workspace(GetContext(), GetContext().GetBrowser());

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

    void Environment::DrawCatalog()
    {
        Toolkit::Composer::SetNextItemWidth(-1.0f);

        if (Toolkit::Composer::BeginCombo("##catalog", ICON_FA_PLUS "  Add Component", ImGuiComboFlags_HeightLarge))
        {
            Text   Group;
            UInt32 Count = 0;

            for (const Scene::Entity Component : mCatalog.GetComponents())
            {
                if (!Component.Has(EcsSingleton))
                {
                    continue;
                }

                const ConstPtr<ComponentType> Info = Component.TryGet<const ComponentType>();

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

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Environment::Apply()
    {
        if (mAction == Action::None)
        {
            return;
        }

        // A data-less tag has no storage to notify, so it can only ever be added or removed.
        const ConstPtr<ComponentType> Info = mSubject.TryGet<const ComponentType>();

        Ref<History> History = GetContext().GetHistory();

        History.Open(mAction == Action::Add ? "Add Component"_Text : "Remove Component"_Text);
        History.CaptureSingleton(mSubject);

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
        History.Close();

        mAction  = Action::None;
        mSubject = Scene::Entity();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Environment::TrackEdit()
    {
        // A field is remembered the moment it is grabbed, which is before a drag has carried it anywhere.
        const Bool Editing = Toolkit::Composer::IsAnyItemActive()
                          && Toolkit::Composer::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        if (Editing == mEditing)
        {
            return;
        }
        mEditing = Editing;

        Ref<History> History = GetContext().GetHistory();

        if (!Editing)
        {
            History.Close();
            return;
        }

        // Which singleton owns the grabbed field is not knowable from here, so the whole panel is remembered;
        // the ones the edit left alone are dropped when the step closes.
        History.Open("Edit");

        for (const Scene::Entity Component : mCatalog.GetComponents())
        {
            if (Component.Has(EcsSingleton))
            {
                History.CaptureSingleton(Component);
            }
        }
    }
}