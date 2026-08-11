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
#include "Tileon.Editor/Toolkit/Widget/Browser.hpp"
#include "Tileon.Editor/Toolkit/Widget/Previewer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Provides tools and functionality for managing and editing the tileset foundry in the editor.
    class Foundry final : public Activity
    {
    public:

        /// \brief Constructs the activity with the specified context.
        ///
        /// \param Context The context associated with this activity.
        Foundry(Ref<Context> Context);

        /// \see Activity::OnDraw()
        void OnDraw() override;

    private:

        /// \brief Draws the list panel of the foundry interface.
        void DrawListPanel();

        /// \brief Draws the left panel of the foundry interface.
        ///
        /// \param Terrain The currently selected terrain.
        /// \param Motif   The motif data associated with the selected terrain.
        void DrawLeftPanel(Ref<Terrain> Terrain, Ref<Motif> Motif);

        /// \brief Draws the animation panel of the foundry interface for the selected motif.
        ///
        /// \param Motif The motif whose animation is being displayed.
        void DrawLeftPanelAnimation(Ref<Motif> Motif);

        /// \brief Draws the right panel of the foundry interface, showing a preview of the selected motif.
        ///
        /// \param Motif The motif to preview, shown as its baked slice beside the sheets it was cut from.
        void DrawRightPanel(ConstRef<Motif> Motif);

        /// \brief Draws the bottom bar of the foundry interface.
        void DrawBottomBar();

        /// \brief Draws a placeholder panel shown when no terrain is selected.
        ///
        /// \param Message The message to display in the placeholder panel.
        void DrawEmptyPanel(Text Message);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Repository>    mRepository;
        Ref<Tileset>       mTileset;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UInt16             mSelection;
        UInt16             mScroll;
        Toolkit::Previewer mPreviewer;
        Toolkit::Browser   mBrowser;
    };
}