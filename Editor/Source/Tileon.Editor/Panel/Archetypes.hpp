// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Tileon.Editor/Panel.hpp"
#include "Tileon.Editor/Inspect/ComponentList.hpp"
#include "Tileon.Editor/Toolkit/Previewer.hpp"
#include "Tileon.World/Repository.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief An activity that provides tools and functionality for managing and editing archetypes in the editor.
    class Archetypes final : public Panel
    {
    public:

        /// \brief Constructs the activity with the specified context.
        ///
        /// \param Context The context associated with this activity.
        Archetypes(Ref<Context> Context);

        /// \see Panel::OnDraw()
        void OnDraw() override;

    private:

        /// \brief Identifies the destructive edit queued while the archetype tree is being walked.
        enum class Operation : UInt8
        {
            None,       ///< No edit is pending.
            Delete,     ///< Remove the archetype from the repository.
            Detach,     ///< Remove the archetype from its parent.
        };

        /// \brief Draws the list panel of the archetypes interface.
        void DrawListPanel();

        /// \brief Draws a single archetype as a tree node and, recursively, each of its children.
        ///
        /// \param Archetype The archetype entity to draw.
        void DrawArchetypeNode(Scene::Archetype Archetype);

        /// \brief Draws the details panel of the archetypes interface.
        void DrawDetailsPanel();

        /// \brief Draws the preview panel of the archetypes interface, showing how the selected archetype looks.
        void DrawPreviewPanel();

        /// \brief Draws the bottom bar of the archetypes interface.
        void DrawStatusBar();

        /// \brief Creates a new archetype, optionally parented under another, and selects it.
        ///
        /// \param Parent The archetype to parent the new one under, or an invalid entity for a root archetype.
        void CreateArchetype(Scene::Archetype Parent);

        /// \brief Applies the queued destructive edit, if any, and releases every reference into the doomed subtree.
        void FlushDeferOperation();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Repository>                           mRepository;
        ComponentList                             mComponents;
        Table<UInt64, Sequence<Scene::Archetype>> mAdjacency;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Toolkit::Previewer                        mPreview;
        Scene::Archetype                          mSelection;
        Scene::Archetype                          mScroll;
        Scene::Archetype                          mPending;
        Operation                                 mOperation;
    };
}