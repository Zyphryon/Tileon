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

#include "Tileon.Editor/Activity.hpp"
#include "Tileon.Editor/Component/Assembler.hpp"
#include "Tileon.Editor.UI/Widget/Previewer.hpp"
#include "Tileon.World/Repository.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    /// \brief An activity that provides tools and functionality for managing and editing archetypes in the editor.
    class Archetypes final : public Activity
    {
    public:

        /// \brief Constructs the activity with the specified context.
        ///
        /// \param Context The context associated with this activity.
        Archetypes(Ref<Context> Context);

        /// \see Activity::OnDraw(Ref<UI::Composer>)
        void OnDraw(Ref<UI::Composer> Composer) override;

    private:

        /// \brief Identifies the destructive edit queued while the archetype tree is being walked.
        enum class Operation : UInt8
        {
            None,       ///< No edit is pending.
            Delete,     ///< Remove the archetype from the repository.
            Detach,     ///< Remove the archetype from its parent.
        };

        /// \brief Draws the list panel of the archetypes interface.
        ///
        /// \param Composer The UI composer used to render the list panel elements.
        void DrawListPanel(Ref<UI::Composer> Composer);

        /// \brief Draws a single archetype as a tree node and, recursively, each of its children.
        ///
        /// \param Composer  The UI composer used to render the node.
        /// \param Archetype The archetype entity to draw.
        void DrawArchetypeNode(Ref<UI::Composer> Composer, Scene::Archetype Archetype);

        /// \brief Draws the details panel of the archetypes interface.
        ///
        /// \param Composer The UI composer used to render the details panel elements.
        void DrawDetailsPanel(Ref<UI::Composer> Composer);

        /// \brief Draws the preview panel of the archetypes interface, showing how the selected archetype looks.
        ///
        /// \param Composer The UI composer used to render the preview panel elements.
        void DrawPreviewPanel(Ref<UI::Composer> Composer);

        /// \brief Draws the bottom bar of the archetypes interface.
        ///
        /// \param Composer The UI composer used to render the bottom bar elements.
        void DrawBottomBar(Ref<UI::Composer> Composer);

        /// \brief Draws an empty panel with a message.
        ///
        /// \param Composer The UI composer used to render the empty panel elements.
        /// \param Message  The message to display in the empty panel.
        void DrawEmptyPanel(Ref<UI::Composer> Composer, Text Message);

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
        Assembler                                 mAssembler;
        Table<UInt64, Sequence<Scene::Archetype>> mAdjacency;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UI::Previewer                             mPreviewer;
        Scene::Archetype                          mSelection;
        Scene::Archetype                          mScroll;
        Scene::Archetype                          mPending;
        Operation                                 mOperation;
    };
}


