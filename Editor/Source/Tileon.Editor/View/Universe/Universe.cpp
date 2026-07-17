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

namespace Tileon::Editor::View
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Universe::Universe(Ref<Context> Context)
        : Activity  { Context, "Universe" },
          mRegistry { Context.GetRegistry() },
          mAction   { Action::None }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Universe::OnDraw(Ref<UI::Composer> Composer)
    {
        Composer.SetNextWindowSize(320.0f, 480.0f, ImGuiCond_FirstUseEver);

        if (Composer.Begin(GetTitle(), mVisible))
        {
            constexpr Real32 kFooterHeight = 40.0f;

            mAction  = Action::None;
            mSubject = Scene::Entity();

            Composer.BeginChild("##body", ImVec2(0, -kFooterHeight - ImGui::GetStyle().ItemSpacing.y), ImGuiChildFlags_Borders);
            Composer.Section("Components");

            UInt32 Count = 0;

            for (const Scene::Entity Component : mRegistry.GetCatalog())
            {
                if (Component.Has(flecs::Singleton))
                {
                    DrawComponent(Composer, Component);

                    ++Count;
                }
            }

            if (Count == 0)
            {
                DrawEmptyPanel(Composer, "The world holds no components");
            }
            Composer.EndChild();

            Composer.BeginChild("##footer", ImVec2(0, kFooterHeight));
            DrawCatalog(Composer);
            Composer.EndChild();

            Apply();
        }
        Composer.End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Universe::DrawComponent(Ref<UI::Composer> Composer, Scene::Entity Component)
    {
        const ConstPtr<Descriptor> Info  = Component.TryGet<const Descriptor>();
        const String<128>          Label = String<128>::Print<"{0}  {1}##{2}">(Info->GetIcon(), Info->GetLabel(), Component.GetID());

        const Bool Open = Composer.TreeNode(Label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);

        if (Composer.BeginPopupContextItem())
        {
            if (Composer.MenuItem("Remove"))
            {
                mAction  = Action::Remove;
                mSubject = Component;
            }
            Composer.EndPopup();
        }

        if (Open)
        {
            if (Info->HasFields())
            {
                if (Info->Inspect(Composer, GetContext(), Component, Component.TryGet(Component)))
                {
                    Component.Notify(Component);
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

    void Universe::DrawCatalog(Ref<UI::Composer> Composer)
    {
        Composer.SetNextItemWidth(-1.0f);

        if (Composer.BeginCombo("##catalog", ICON_FA_PLUS "  Add Component", ImGuiComboFlags_HeightLarge))
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
                    Composer.Section(Group);
                }

                const Bool        Owned = Component.Has(Component);
                const String<128> Label = String<128>::Print<"{0}  {1}">(Info->GetIcon(), Info->GetLabel());

                Composer.BeginDisabled(Owned);

                if (Composer.Selectable(Label) && !Owned)
                {
                    mAction  = Action::Add;
                    mSubject = Component;
                }

                Composer.EndDisabled();

                ++Count;
            }

            if (Count == 0)
            {
                Composer.TextDisabled("No components are registered as singletons");
            }
            Composer.EndCombo();
        }
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

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Universe::DrawEmptyPanel(Ref<UI::Composer> Composer, Text Message)
    {
        const ImVec2 Available = Composer.GetContentRegionAvail();
        const ImVec2 HintSize  = Composer.CalcTextSize(Message);

        Composer.SetCursorPosX((Available.x - HintSize.x) * 0.5f);
        Composer.SetCursorPosY((Available.y - HintSize.y) * 0.5f);
        Composer.TextDisabled(Message);
    }
}
