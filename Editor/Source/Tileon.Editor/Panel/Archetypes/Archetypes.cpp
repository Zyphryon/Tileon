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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Panel
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool IsAncestor(Scene::Archetype Node, Scene::Archetype Candidate)
    {
        for (Scene::Archetype Walk = Node.GetParent(); Walk.IsValid(); Walk = Walk.GetParent())
        {
            if (Walk == Candidate)
            {
                return true;
            }
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool IsInheritedFrom(Scene::Archetype Node, Scene::Archetype Candidate)
    {
        for (Scene::Archetype Walk = Node.GetArchetype(); Walk.IsValid(); Walk = Walk.GetArchetype())
        {
            if (Walk == Candidate)
            {
                return true;
            }
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Archetypes::Archetypes(Ref<Context> Context)
        : Activity    { Context, "Archetypes" },
          mRepository { Context.GetRepository() },
          mAssembler  { Context },
          mOperation  { Operation::None }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::OnDraw(Ref<UI::Composer> Composer)
    {
        // Adopt an archetype requested from another panel (Palette right-click), scroll it into view, and pull focus.
        if (const SInt64 Request = GetContext().GetInteger("Selection.Archetype.Target", 0); Request != 0)
        {
            mSelection = mRepository.GetArchetype(static_cast<UInt64>(Request));
            mScroll    = mSelection;
            mPreviewer.Reset();

            GetContext().SetInteger("Selection.Archetype.Target", 0);
            ImGui::SetNextWindowFocus();
        }

        Composer.SetNextWindowSize(1100.0f, 620.0f, ImGuiCond_FirstUseEver);
        Composer.SetNextWindowSizeConstraints(560.0f, 360.0f, 1800.0f, 1400.0f);

        if (Composer.Begin(GetTitle(), mVisible))
        {
            const Real32 BodyHeight = -(Composer.GetFrameHeightWithSpacing() + 8.0f);

            Composer.BeginChild("##list_panel", ImVec2(260.0f, BodyHeight), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders);
            DrawListPanel(Composer);
            Composer.EndChild();

            if (mSelection.IsValid())
            {
                Composer.SameLine();
                Composer.BeginChild("##details_panel", ImVec2(380.0f, BodyHeight), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders);
                DrawDetailsPanel(Composer);
                Composer.EndChild();

                Composer.SameLine();
                Composer.BeginChild("##preview_panel", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_Borders);
                DrawPreviewPanel(Composer);
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

        // The browser is modal, so it is drawn outside the window that hosts the fields which opened it.
        mAssembler.DrawSelector(Composer);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawListPanel(Ref<UI::Composer> Composer)
    {
        if (Composer.Button(ICON_FA_PLUS "  Archetype", -1.0f))
        {
            CreateArchetype(Scene::Archetype());
        }

        Composer.Separator();
        Composer.BeginChild("##list_scroll");

        // Group every archetype under its parent in a single pass for O(N) walk of cached lists instead of the O(N^2)
        mAdjacency.Clear();

        mRepository.ForEachArchetype([&](Scene::Archetype Archetype)
        {
            mAdjacency.FindOrInsert(Archetype.GetParent().GetID()).Append(Archetype);
        });

        // Draw the forest rooted at every archetype that has no parent; children are visited recursively.
        if (const ConstPtr<Sequence<Scene::Archetype>> Roots = mAdjacency.Find<UInt64>(0))
        {
            for (const Scene::Archetype Root : * Roots)
            {
                DrawArchetypeNode(Composer, Root);
            }
        }
        FlushDeferOperation();

        Composer.EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawArchetypeNode(Ref<UI::Composer> Composer, Scene::Archetype Archetype)
    {
        // Resolve children from the pre-built adjacency so leaves render without an expander arrow.
        const ConstPtr<Sequence<Scene::Archetype>> Children    = mAdjacency.Find(Archetype.GetID());
        const Bool                                 HasChildren = (Children && !Children->IsEmpty());

        const Bool Selected = (mSelection == Archetype);

        ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth
                                 | ImGuiTreeNodeFlags_DefaultOpen;

        if (!HasChildren)
        {
            Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }
        if (Selected)
        {
            Flags |= ImGuiTreeNodeFlags_Selected;
        }

        const Text        Icon  = HasChildren ? ICON_FA_CUBES : ICON_FA_CUBE;
        const String<128> Label = String<128>::Print<"{0}  {1}###{2:016X}">(Icon, Archetype.GetAlias(), Archetype.GetID());

        const Bool Open = Composer.TreeNode(Label, Flags);

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && mSelection != Archetype)
        {
            mSelection = Archetype;

            mPreviewer.Reset();
        }

        // Bring a freshly created archetype into view once, then clear the request so manual scrolling is left alone.
        if (mScroll == Archetype)
        {
            mScroll = Scene::Archetype();
            Composer.SetScrollHereY(0.5f);
        }

        // Dragging a node onto another re-parents it, allowing prefab hierarchies to be built by hand.
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            const UInt64 Payload = Archetype.GetID();
            ImGui::SetDragDropPayload("ARCHETYPE_NODE", &Payload, sizeof(Payload));
            Composer.Label("{0}  {1}", Icon, Archetype.GetAlias());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ConstPtr<ImGuiPayload> Data = ImGui::AcceptDragDropPayload("ARCHETYPE_NODE"))
            {
                const Scene::Archetype Source = mRepository.GetArchetype(* static_cast<ConstPtr<UInt64>>(Data->Data));

                // Reject no-ops and any move that would place an archetype under one of its own descendants.
                if (Source.IsValid() && Source != Archetype && Source.GetParent() != Archetype && !IsAncestor(Archetype, Source))
                {
                    mRepository.AttachArchetype(Archetype, Source);
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (Composer.BeginPopupContextItem())
        {
            if (Composer.MenuItem("New Child"))
            {
                CreateArchetype(Archetype);
            }

            Composer.Separator();

            if (Composer.MenuItem("Detach from Parent", { }, Archetype.GetParent().IsValid()))
            {
                mPending   = Archetype;
                mOperation = Operation::Detach;
            }

            if (Composer.MenuItem("Delete"))
            {
                mPending   = Archetype;
                mOperation = Operation::Delete;
            }

            if (Composer.MenuItem("Refresh"))
            {
                mRepository.RefreshArchetype(mSelection);
            }
            Composer.EndPopup();
        }

        if (Open && HasChildren)
        {
            for (const Scene::Archetype Child : * Children)
            {
                DrawArchetypeNode(Composer, Child);
            }

            Composer.TreePop();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawDetailsPanel(Ref<UI::Composer> Composer)
    {
        Composer.Section("Identity");

        Composer.Field("ID");
        Composer.Label("{0:016X}", mSelection.GetID());
        Composer.Spacing();

        Composer.Field("Name");
        Composer.SetNextItemWidth(-1.0f);
        Composer.InputText("##archetype_name", mSelection.GetName(), [&](Text Value)
        {
            mSelection.SetName(Value);
        });
        Composer.Spacing();

        Composer.Field("Alias");
        Composer.SetNextItemWidth(-1.0f);
        Composer.InputText("##archetype_display", mSelection.GetAlias(), [&](Text Value)
        {
            mSelection.SetAlias(Value);
        });
        Composer.Spacing();

        Composer.Field("Archetype");
        Composer.SetNextItemWidth(-1.0f);

        const Scene::Archetype Base = mSelection.GetArchetype();

        if (Composer.BeginCombo("##archetype_base", Base.IsValid() ? Base.GetAlias() : "None"_Text))
        {
            if (Composer.Selectable("None", !Base.IsValid()))
            {
                mSelection.GetEntity().SetArchetype(Scene::Entity());
            }

            mRepository.ForEachArchetype([&](Scene::Archetype Candidate)
            {
                // Exclude self and anything that already inherits from the selection, which would form a cycle.
                if (Candidate == mSelection || IsInheritedFrom(Candidate, mSelection))
                {
                    return;
                }

                const String<128> Label = String<128>::Print<"{0}##{1:016X}">(Candidate.GetAlias(), Candidate.GetID());

                if (Composer.Selectable(Label, Candidate == Base))
                {
                    mSelection.GetEntity().SetArchetype(Candidate.GetEntity());
                }
            });

            Composer.EndCombo();
        }
        Composer.Spacing();

        Composer.Section("Hierarchy");

        Composer.Field("Parent");

        if (const Scene::Archetype Parent = mSelection.GetParent(); Parent.IsValid())
        {
            Composer.Label(Parent.GetAlias());
            Composer.Spacing();

            if (Composer.Button(ICON_FA_LINK_SLASH "  Detach", -1.0f))
            {
                mPending   = mSelection;
                mOperation = Operation::Detach;
            }
        }
        else
        {
            Composer.TextDisabled("None (root)");
        }
        Composer.Spacing();

        Composer.Section("Components");

        mAssembler.Draw(Composer, mSelection.GetEntity());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawPreviewPanel(Ref<UI::Composer> Composer)
    {
        const Scene::Entity        Actor  = mSelection.GetEntity();
        const ConstPtr<Appearance> Visual = Actor.TryGet<const Appearance>();

        if (!Visual)
        {
            DrawEmptyPanel(Composer, Actor.Has<Typeface>()
                ? "Text preview unavailable"_Text
                : "This archetype has nothing to preview"_Text);
            return;
        }

        ConstRetainer<Graphic::Material> Material = Visual->GetMaterial();

        if (!Material)
        {
            DrawEmptyPanel(Composer, "No material assigned to this archetype");
            return;
        }

        if (!Material->HasCompleted())
        {
            DrawEmptyPanel(Composer, Material->HasFailed() ? "Material failed to load"_Text : "Loading material..."_Text);
            return;
        }

        if (Composer.BeginTabBar("##preview_tabs"))
        {
            if (ConstRetainer<Graphic::Image> Albedo = Material->GetImage(Graphic::TextureSlot::Albedo))
            {
                if (Composer.BeginTabItem("Preview"))
                {
                    const Rect    Source = Visual->GetSource();
                    const Vector2 Size(Source.GetWidth() * Albedo->GetWidth(), Source.GetHeight() * Albedo->GetHeight());

                    const ConstPtr<IntColor8> Tint = Actor.TryGet<const IntColor8>();

                    mPreviewer.Draw(Composer, Albedo->GetHandle(), Size, Source, Tint ? Color::FromColor8(* Tint) : Color::White());

                    Composer.EndTabItem();
                }
            }

            for (const Graphic::TextureSlot Semantic : Enum::GetValues<Graphic::TextureSlot>())
            {
                if (ConstRetainer<Graphic::Image> Texture = Material->GetImage(Semantic))
                {
                    if (Composer.BeginTabItem(Enum::GetName(Semantic)))
                    {
                        const Vector2 Size(Texture->GetWidth(), Texture->GetHeight());
                        mPreviewer.Draw(Composer, Texture->GetHandle(), Size, Rect::One());

                        Composer.EndTabItem();
                    }
                }
            }

            Composer.EndTabBar();
        }
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
            const Text Label = mSelection.GetAlias();

            Composer.SetCursorPosX((Composer.GetWindowWidth() - Composer.CalcTextSize(Label).x) * 0.5f);
            Composer.TextDisabled(Label);
        }
        else
        {
            constexpr Text Hint = "No archetype selected";

            Composer.SetCursorPosX((Composer.GetWindowWidth() - Composer.CalcTextSize(Hint).x) * 0.5f);
            Composer.TextDisabled(Hint);
        }

        Composer.EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawEmptyPanel(Ref<UI::Composer> Composer, Text Message)
    {
        constexpr Text kIcon = "?";

        const ImVec2 Available = Composer.GetContentRegionAvail();
        const ImVec2 IconSize  = Composer.CalcTextSize(kIcon);
        const ImVec2 HintSize  = Composer.CalcTextSize(Message);

        Composer.SetCursorPosX((Available.x - IconSize.x) * 0.5f);
        Composer.SetCursorPosY((Available.y - IconSize.y + 8.0f + HintSize.y) * 0.5f);
        Composer.TextDisabled(kIcon);

        Composer.SetCursorPosX((Available.x - HintSize.x) * 0.5f);
        Composer.TextDisabled(Message);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::CreateArchetype(Scene::Archetype Parent)
    {
        if (const Scene::Archetype Archetype = mRepository.CreateArchetype(); Archetype.IsValid())
        {
            Archetype.SetAlias(Str32::Print<"Archetype.{0}">(Archetype.GetID() - Scene::kMinRangeArchetypes));

            if (Parent.IsValid())
            {
                mRepository.AttachArchetype(Parent, Archetype);
            }

            mSelection = Archetype;
            mScroll    = Archetype;
            mPreviewer.Reset();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::FlushDeferOperation()
    {
        if (!mPending.IsValid())
        {
            return;
        }

        if (mSelection.IsValid() && (mSelection == mPending || IsAncestor(mSelection, mPending)))
        {
            mSelection = Scene::Archetype();

            mPreviewer.Reset();
        }
        if (mScroll.IsValid() && (mScroll == mPending || IsAncestor(mScroll, mPending)))
        {
            mScroll = Scene::Archetype();
        }

        switch (mOperation)
        {
        case Operation::Delete:
            mRepository.DeleteArchetype(mPending);
            break;
        case Operation::Detach:
            // TODO
            break;
        case Operation::None:
            break;
        }

        mPending   = Scene::Archetype();
        mOperation = Operation::None;
    }
}

