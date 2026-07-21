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
#include "Gizmo.hpp"
#include "Workshop.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    /// \brief Responsible for rendering the game world and providing tools for editing and manipulating the scene.
    class Atelier final : public Activity
    {
    public:

        /// \brief Static name identifier for the atelier activity.
        static constexpr Symbol kTitle = "Atelier";

    public:

        /// \brief Constructs the activity with the specified context.
        ///
        /// \param Context The context associated with this activity.
        Atelier(Ref<Context> Context);

        /// \see Activity::OnDraw(Ref<UI::Composer>)
        void OnDraw(Ref<UI::Composer> Composer) override;

    private:

        /// \brief Draws the toolbar for the scene activity.
        ///
        /// \param Composer The UI composer used to render the toolbar elements.
        void DrawToolbar(Ref<UI::Composer> Composer);

        /// \brief Draws a toolbar button that selects the given brush, highlighted while that brush is active.
        ///
        /// \param Composer The UI composer used to render the button.
        /// \param Brush    The brush the button selects.
        /// \param Icon     The icon displayed on the button.
        /// \param Tooltip  The hint shown when the button is hovered.
        void DrawBrushButton(Ref<UI::Composer> Composer, Workshop::Brush Brush, Text Icon, Text Tooltip);

        /// \brief Draws a toolbar button that toggles the given overlay, highlighted while that overlay is enabled.
        ///
        /// \param Composer The UI composer used to render the button.
        /// \param Overlay  The diagnostic overlay the button toggles.
        /// \param Icon     The icon displayed on the button.
        /// \param Tooltip  The hint shown when the button is hovered.
        void DrawDebugButton(Ref<UI::Composer> Composer, Renderer::Debug Overlay, Text Icon, Text Tooltip);

        /// \brief Draws the tile editing toolbar for the scene activity.
        ///
        /// \param Composer The UI composer used to render the tile editing toolbar elements.
        void DrawTileToolbar(Ref<UI::Composer> Composer);

        /// \brief Draws the entity editing toolbar for the scene activity.
        ///
        /// \param Composer The UI composer used to render the entity editing toolbar elements.
        void DrawEntityToolbar(Ref<UI::Composer> Composer);

        /// \brief Draws the viewport for the scene activity, displaying the rendered game world.
        ///
        /// \param Composer The UI composer used to render the viewport elements.
        void DrawViewport(Ref<UI::Composer> Composer);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Workshop         mWorkshop;
        Gizmo            mGizmo;
        Renderer::Target mTarget;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Bool             mMarquee;
        Bool             mMarqueeMoved;
        ImVec2           mMarqueeScreen;
        SInt32           mPaintTileX;
        SInt32           mPaintTileY;
    };
}