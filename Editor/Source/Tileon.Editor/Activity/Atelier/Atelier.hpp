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
#include "Lens.hpp"
#include "Workshop.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Responsible for rendering the game world and providing tools for editing and manipulating the scene.
    class Atelier final : public Activity
    {
    public:

        /// \brief Static name identifier for the atelier activity.
        static constexpr Symbol kTitle   = "Atelier";

        /// \brief How far the ground may tilt away from edge-on.
        static constexpr Real32 kMinTilt = 0.15f;

        /// \brief How far the ground may tilt toward flat on.
        static constexpr Real32 kMaxTilt = 1.0f;

        /// \brief Enumerates the projections the viewport offers.
        enum class Perspective : UInt8
        {
            Ortho,          ///< Views the ground straight on, with nothing foreshortened.
            Isometric,      ///< Views the ground at a two-to-one angle.
            Axonometric,    ///< Views the ground from an angle the cursor swings around.
        };

    public:

        /// \brief Constructs the activity with the specified context.
        ///
        /// \param Context The context associated with this activity.
        Atelier(Ref<Context> Context);

        /// \see Activity::OnDraw()
        void OnDraw() override;

    private:

        /// \brief Draws the toolbar for the scene activity.
        void DrawToolbar();

        /// \brief Draws a toolbar button that selects the given brush, highlighted while that brush is active.
        ///
        /// \param Brush   The brush the button selects.
        /// \param Icon    The icon displayed on the button.
        /// \param Tooltip The hint shown when the button is hovered.
        void DrawBrushButton(Workshop::Brush Brush, Text Icon, Text Tooltip);

        /// \brief Draws a toolbar button that toggles the given overlay, highlighted while that overlay is enabled.
        ///
        /// \param Overlay The diagnostic overlay the button toggles.
        /// \param Icon    The icon displayed on the button.
        /// \param Tooltip The hint shown when the button is hovered.
        void DrawDebugButton(Renderer::Debug Overlay, Text Icon, Text Tooltip);

        /// \brief Draws the pause toggle and speed selector the scene simulation runs at.
        void DrawTimescaleToolbar();

        /// \brief Outlines the entity a click would select.
        ///
        /// \param Lens  The viewport the outline is drawn into.
        /// \param Actor The entity a click would take, if any.
        void DrawSelectionHint(ConstRef<Lens> Lens, Scene::Entity Actor);

        /// \brief Draws the tile editing toolbar for the scene activity.
        void DrawTileToolbar();

        /// \brief Draws the entity editing toolbar for the scene activity.
        void DrawEntityToolbar();

        /// \brief Draws the viewport for the scene activity, displaying the rendered game world.
        void DrawViewport();

        /// \brief Marks every light in the scene, which is otherwise invisible until it is selected.
        ///
        /// \param Lens The lens that projects world placements into the viewport.
        void DrawLightMarkers(ConstRef<Lens> Lens);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Workshop         mWorkshop;
        Gizmo            mGizmo;
        Renderer::Target mTarget;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Scene::Query     mQrGlowlights;
        Scene::Query     mQrSpotlights;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Real32           mTimescale;
        Real32           mYaw;
        Real32           mTilt;
        Bool             mMarquee;
        Bool             mMarqueeMoved;
        ImVec2           mMarqueeScreen;
        SInt32           mPaintTileX;
        SInt32           mPaintTileY;
    };
}