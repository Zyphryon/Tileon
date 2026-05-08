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

        /// \copydoc Activity::OnDraw(Ref<UI::Composer>)
        void OnDraw(Ref<UI::Composer> Composer) override;

    private:

        /// \brief Draws the list panel of the archetypes interface.
        ///
        /// \param Composer The UI composer used to render the list panel elements.
        void DrawListPanel(Ref<UI::Composer> Composer);

        /// \brief Draws the details panel of the archetypes interface.
        ///
        /// \param Composer The UI composer used to render the details panel elements.
        void DrawDetailsPanel(Ref<UI::Composer> Composer);

        /// \brief Draws the bottom bar of the archetypes interface.
        ///
        /// \param Composer The UI composer used to render the bottom bar elements.
        void DrawBottomBar(Ref<UI::Composer> Composer);

        /// \brief Draws an empty panel with a message.
        ///
        /// \param Composer The UI composer used to render the empty panel elements.
        /// \param Message  The message to display in the empty panel.
        void DrawEmptyPanel(Ref<UI::Composer> Composer, ConstStr8 Message);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Repository> mRepository;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Scene::Entity   mSelection;
    };
}


