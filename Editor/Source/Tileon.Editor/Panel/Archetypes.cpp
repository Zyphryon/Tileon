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
#include "Tileon.Editor/Utility.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
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
        : Panel       { Context, "Archetypes" },
          mRepository { Context.GetRepository() },
          mComponents { Context },
          mOperation  { Operation::None }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::OnDraw()
    {
        // Adopt an archetype requested from another panel (Palette right-click), scroll it into view, and pull focus.
        if (const SInt64 Request = GetContext().GetInteger("Selection.Archetype.Target", 0); Request != 0)
        {
            mSelection = mRepository.GetArchetype(static_cast<UInt64>(Request));
            mScroll    = mSelection;
            mPreview.Reset();

            GetContext().SetInteger("Selection.Archetype.Target", 0);
            Toolkit::Composer::SetNextWindowFocus();
        }

        Toolkit::Composer::SetNextWindowSize(1100.0f, 620.0f, ImGuiCond_FirstUseEver);
        Toolkit::Composer::SetNextWindowSizeConstraints(560.0f, 360.0f, 1800.0f, 1400.0f);

        if (Toolkit::Composer::Begin(GetTitle(), mVisible))
        {
            const Real32 BodyHeight = -(Toolkit::Composer::GetFrameHeightWithSpacing() + 8.0f);

            Toolkit::Composer::BeginChild("##list_panel", ImVec2(260.0f, BodyHeight), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders);
            DrawListPanel();
            Toolkit::Composer::EndChild();

            if (mSelection.IsValid())
            {
                Toolkit::Composer::SameLine();
                Toolkit::Composer::BeginChild("##details_panel", ImVec2(380.0f, BodyHeight), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders);
                DrawDetailsPanel();
                Toolkit::Composer::EndChild();

                Toolkit::Composer::SameLine();
                Toolkit::Composer::BeginChild("##preview_panel", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_Borders);
                DrawPreviewPanel();
                Toolkit::Composer::EndChild();
            }
            else
            {
                Toolkit::Composer::SameLine();
                Toolkit::Composer::BeginChild("##empty_panel", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_Borders);
                DrawEmptyPanel("No archetype selected", "?");
                Toolkit::Composer::EndChild();
            }

            DrawStatusBar();
        }
        Toolkit::Composer::End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawListPanel()
    {
        if (Toolkit::Composer::Button(ICON_FA_PLUS "  Archetype", -1.0f))
        {
            CreateArchetype(Scene::Archetype());
        }

        Toolkit::Composer::Separator();
        Toolkit::Composer::BeginChild("##list_scroll");

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
                DrawArchetypeNode(Root);
            }
        }
        FlushDeferOperation();

        Toolkit::Composer::EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawArchetypeNode(Scene::Archetype Archetype)
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

        const Bool Open = Toolkit::Composer::TreeNode(Label, Flags);

        if (Toolkit::Composer::IsItemClicked() && !Toolkit::Composer::IsItemToggledOpen() && mSelection != Archetype)
        {
            mSelection = Archetype;

            mPreview.Reset();
        }

        // Bring a freshly created archetype into view once, then clear the request so manual scrolling is left alone.
        if (mScroll == Archetype)
        {
            mScroll = Scene::Archetype();
            Toolkit::Composer::SetScrollHereY(0.5f);
        }

        // Dragging a node onto another re-parents it, allowing prefab hierarchies to be built by hand.
        if (Toolkit::Composer::BeginDragDropSource())
        {
            const UInt64 Payload = Archetype.GetID();
            Toolkit::Composer::SetDragDropPayload("ARCHETYPE_NODE", Payload);
            Toolkit::Composer::Label("{0}  {1}", Icon, Archetype.GetAlias());
            Toolkit::Composer::EndDragDropSource();
        }

        if (Toolkit::Composer::BeginDragDropTarget())
        {
            if (const ConstPtr<UInt64> Data = Toolkit::Composer::AcceptDragDropPayload<UInt64>("ARCHETYPE_NODE"))
            {
                const Scene::Archetype Source = mRepository.GetArchetype(* Data);

                // Reject no-ops and any move that would place an archetype under one of its own descendants.
                if (Source.IsValid() && Source != Archetype && Source.GetParent() != Archetype && !IsAncestor(Archetype, Source))
                {
                    mRepository.AttachArchetype(Archetype, Source);
                }
            }
            Toolkit::Composer::EndDragDropTarget();
        }

        if (Toolkit::Composer::BeginPopupContextItem())
        {
            if (Toolkit::Composer::MenuItem("New Child"))
            {
                CreateArchetype(Archetype);
            }

            Toolkit::Composer::Separator();

            if (Toolkit::Composer::MenuItem("Detach from Parent", { }, Archetype.GetParent().IsValid()))
            {
                mPending   = Archetype;
                mOperation = Operation::Detach;
            }

            if (Toolkit::Composer::MenuItem("Delete"))
            {
                mPending   = Archetype;
                mOperation = Operation::Delete;
            }

            if (Toolkit::Composer::MenuItem("Refresh"))
            {
                mRepository.RefreshArchetype(mSelection);
            }
            Toolkit::Composer::EndPopup();
        }

        if (Open && HasChildren)
        {
            for (const Scene::Archetype Child : * Children)
            {
                DrawArchetypeNode(Child);
            }

            Toolkit::Composer::TreePop();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawDetailsPanel()
    {
        Toolkit::Composer::Section("Identity");

        Toolkit::Composer::Field("ID");
        Toolkit::Composer::Label("{0:016X}", mSelection.GetID());
        Toolkit::Composer::Spacing();

        Toolkit::Composer::Field("Name");
        Toolkit::Composer::SetNextItemWidth(-1.0f);
        Toolkit::Composer::InputText("##archetype_name", mSelection.GetName(), [&](Text Value)
        {
            mSelection.SetName(Value);
        });
        Toolkit::Composer::Spacing();

        Toolkit::Composer::Field("Alias");
        Toolkit::Composer::SetNextItemWidth(-1.0f);
        Toolkit::Composer::InputText("##archetype_display", mSelection.GetAlias(), [&](Text Value)
        {
            mSelection.SetAlias(Value);
        });
        Toolkit::Composer::Spacing();

        Toolkit::Composer::Field("Archetype");
        Toolkit::Composer::SetNextItemWidth(-1.0f);

        const Scene::Archetype Base = mSelection.GetArchetype();

        if (Toolkit::Composer::BeginCombo("##archetype_base", Base.IsValid() ? Base.GetAlias() : "None"_Text))
        {
            if (Toolkit::Composer::Selectable("None", !Base.IsValid()))
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

                if (Toolkit::Composer::Selectable(Label, Candidate == Base))
                {
                    mSelection.GetEntity().SetArchetype(Candidate.GetEntity());
                }
            });

            Toolkit::Composer::EndCombo();
        }
        Toolkit::Composer::Spacing();

        Toolkit::Composer::Section("Hierarchy");

        Toolkit::Composer::Field("Parent");

        if (const Scene::Archetype Parent = mSelection.GetParent(); Parent.IsValid())
        {
            Toolkit::Composer::Label(Parent.GetAlias());
            Toolkit::Composer::Spacing();

            if (Toolkit::Composer::Button(ICON_FA_LINK_SLASH "  Detach", -1.0f))
            {
                mPending   = mSelection;
                mOperation = Operation::Detach;
            }
        }
        else
        {
            Toolkit::Composer::TextDisabled("None (root)");
        }
        Toolkit::Composer::Spacing();

        Toolkit::Composer::Section("Components");

        mComponents.Draw(mSelection.GetEntity());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawPreviewPanel()
    {
        const Scene::Entity        Actor  = mSelection.GetEntity();
        const ConstPtr<Appearance> Visual = Actor.TryGet<const Appearance>();

        if (!Visual)
        {
            DrawEmptyPanel(Actor.Has<Typeface>()
                ? "Text preview unavailable"_Text
                : "This archetype has nothing to preview"_Text, "?");
            return;
        }

        ConstRetainer<Graphic::Material> Material = Visual->GetMaterial();

        if (!Material)
        {
            DrawEmptyPanel("No material assigned to this archetype", "?");
            return;
        }

        if (!Material->HasCompleted())
        {
            DrawEmptyPanel(Material->HasFailed() ? "Material failed to load"_Text : "Loading material..."_Text, "?");
            return;
        }

        if (Toolkit::Composer::BeginTabBar("##preview_tabs"))
        {
            if (ConstRetainer<Graphic::Image> Albedo = Material->GetImage(GetTextureID(Texture::Albedo)))
            {
                if (Toolkit::Composer::BeginTabItem("Preview"))
                {
                    const Rect    Source = Visual->GetSource();
                    const Vector2 Size(Source.GetWidth() * Albedo->GetWidth(), Source.GetHeight() * Albedo->GetHeight());

                    const ConstPtr<IntColor8> Tint = Actor.TryGet<const IntColor8>();

                    mPreview.Draw(Albedo->GetHandle(), Size, Source, Tint ? Color::FromColor8(* Tint) : Color::White());

                    Toolkit::Composer::EndTabItem();
                }
            }

            for (const Texture Semantic : Enum::GetValues<Texture>())
            {
                if (ConstRetainer<Graphic::Image> Texture = Material->GetImage(GetTextureID(Semantic)))
                {
                    if (Toolkit::Composer::BeginTabItem(Enum::GetName(Semantic)))
                    {
                        const Vector2 Size(Texture->GetWidth(), Texture->GetHeight());
                        mPreview.Draw(Texture->GetHandle(), Size, Rect::One());

                        Toolkit::Composer::EndTabItem();
                    }
                }
            }

            Toolkit::Composer::EndTabBar();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Archetypes::DrawStatusBar()
    {
        DrawBottomBar("##status_bar", [&](Real32)
        {
            if (mSelection.IsValid())
            {
                const Text Label = mSelection.GetAlias();

                Toolkit::Composer::SetCursorPosX((Toolkit::Composer::GetWindowWidth() - Toolkit::Composer::CalcTextSize(Label).x) * 0.5f);
                Toolkit::Composer::TextDisabled(Label);
            }
            else
            {
                constexpr Text Hint = "No archetype selected";

                Toolkit::Composer::SetCursorPosX((Toolkit::Composer::GetWindowWidth() - Toolkit::Composer::CalcTextSize(Hint).x) * 0.5f);
                Toolkit::Composer::TextDisabled(Hint);
            }
        });
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
            mPreview.Reset();
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

            mPreview.Reset();
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