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
    /// \brief Provides tools and functionality for managing and editing the tileset atlas in the editor.
    class Atlas final : public Activity
    {
    public:

        /// \brief Constructs the activity with the specified context.
        ///
        /// \param Context The context associated with this activity.
        Atlas(Ref<Context> Context);

        /// \copydoc Activity::OnDraw(Ref<UI::Composer>)
        void OnDraw(Ref<UI::Composer> Composer) override;

    private:

        /// \brief Draws the list panel of the atlas interface.
        ///
        /// \param Composer The UI composer used to render the list panel elements.
        void DrawListPanel(Ref<UI::Composer> Composer);

        /// \brief Draws the left panel of the atlas interface.
        ///
        /// \param Composer The UI composer used to render the left panel elements.
        /// \param Terrain  The currently selected terrain.
        /// \param Motif    The motif data associated with the selected terrain.
        void DrawLeftPanel(Ref<UI::Composer> Composer, Ref<Terrain> Terrain, Ref<Motif> Motif);

        /// \brief Draws the animation panel of the atlas interface for the selected motif.
        ///
        /// \param Composer The UI composer used to render the animation panel elements.
        /// \param Motif    The motif whose animation is being displayed.
        /// \param Glyph    The glyph containing the rendering data for the motif.
        void DrawLeftPanelAnimation(Ref<UI::Composer> Composer, Ref<Motif> Motif, ConstRef<Tileset::Glyph> Glyph);

        /// \brief Draws the right panel of the atlas interface, showing a preview of the selected motif.
        ///
        /// \param Composer The UI composer used to render the right panel elements.
        /// \param Glyph    The glyph containing the rendering data for the motif to preview.
        void DrawRightPanel(Ref<UI::Composer> Composer, ConstRef<Tileset::Glyph> Glyph);

        /// \brief Draws the bottom bar of the atlas interface.
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