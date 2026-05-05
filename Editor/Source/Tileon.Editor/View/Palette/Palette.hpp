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
#include "Tileon.Editor.UI/Widget/Previewer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    /// \brief Provides tools and functionality for managing and editing the tileset palette in the editor.
    class Palette final : public Activity
    {
    public:

        /// \brief Constructs the activity with the specified context.
        ///
        /// \param Context The context associated with this activity.
        Palette(Ref<Context> Context);

        /// \copydoc Activity::OnDraw(Ref<UI::Composer>)
        void OnDraw(Ref<UI::Composer> Composer) override;

    private:

        /// \brief Draws the list panel of the palette interface.
        ///
        /// \param Composer The UI composer used to render the list panel elements.
        void DrawListPanel(Ref<UI::Composer> Composer);

        /// \brief Draws the left panel of the palette interface.
        ///
        /// \param Composer The UI composer used to render the left panel elements.
        /// \param Terrain  The currently selected terrain.
        /// \param Entry    The tileset entry data associated with the selected terrain.
        void DrawLeftPanel(Ref<UI::Composer> Composer, Ref<Terrain> Terrain, Ref<Tileset::Entry> Entry);

        /// \brief Draws the animation section inside the left panel.
        ///
        /// \param Composer The UI composer used to render the animation section elements.
        /// \param Entry    The tileset entry data associated with the selected terrain.
        void DrawLeftPanelAnimationSection(Ref<UI::Composer> Composer, Ref<Tileset::Entry> Entry);

        /// \brief Draws the right panel of the palette interface.
        ///
        /// \param Composer The UI composer used to render the right panel elements.
        /// \param Terrain  The currently selected terrain.
        /// \param Entry    The tileset entry data associated with the selected terrain.
        void DrawRightPanel(Ref<UI::Composer> Composer, Ref<Terrain> Terrain, Ref<Tileset::Entry> Entry);

        /// \brief Draws the bottom bar of the palette interface.
        ///
        /// \param Composer The UI composer used to render the bottom bar elements.
        void DrawBottomBar(Ref<UI::Composer> Composer);

        /// \brief Draws a placeholder panel shown when no terrain is selected.
        ///
        /// \param Composer The UI composer used to render the placeholder.
        /// \param Message  The message to display in the placeholder panel.
        void DrawEmptyPanel(Ref<UI::Composer> Composer, ConstStr8 Message);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Repository> mRepository;
        Ref<Tileset>    mTileset;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UInt16          mSelection;
        UI::Previewer   mPreviewer;
    };
}