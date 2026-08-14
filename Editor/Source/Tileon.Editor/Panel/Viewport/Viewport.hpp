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
#include "Tileon.Editor/Toolkit/Theme.hpp"
#include "Gizmo.hpp"
#include "Camera.hpp"
#include "Tools.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Responsible for rendering the game world and providing tools for editing and manipulating the scene.
    class Viewport final : public Panel
    {
    public:

        /// \brief Static name identifier for the viewport panel.
        static constexpr Symbol kTitle   = "Viewport";

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
        Viewport(Ref<Context> Context);

        /// \see Panel::OnDraw()
        void OnDraw() override;

    private:

        /// \brief Draws the toolbar for the scene activity.
        void DrawToolbar();

        /// \brief Draws a toolbar button that selects the given brush, highlighted while that brush is active.
        ///
        /// \param Brush   The brush the button selects.
        /// \param Icon    The icon displayed on the button.
        /// \param Tooltip The hint shown when the button is hovered.
        void DrawBrushButton(Tools::Brush Brush, Text Icon, Text Tooltip);

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
        /// \param Camera  The viewport the outline is drawn into.
        /// \param Actor The entity a click would take, if any.
        void DrawSelectionHint(ConstRef<Camera> Camera, Scene::Entity Actor);

        /// \brief Draws the tile editing toolbar for the scene activity.
        void DrawTileToolbar();

        /// \brief Draws the entity editing toolbar for the scene activity.
        void DrawEntityToolbar();

        /// \brief Draws the viewport for the scene activity, displaying the rendered game world.
        void DrawViewport();

        /// \brief Marks every light in the scene, which is otherwise invisible until it is selected.
        ///
        /// \param Camera The camera that projects world placements into the viewport.
        void DrawLightMarkers(ConstRef<Camera> Camera);

        /// \brief Outlines a circle lying in world space, swept about a center by two spanning axes.
        ///
        /// \param Camera   The camera that projects world placements into the viewport.
        /// \param Center The center of the circle, in absolute tiles.
        /// \param AxisU  The first unit axis the circle is swept around.
        /// \param AxisV  The second unit axis the circle is swept around.
        /// \param Radius The radius of the circle, in tiles.
        /// \param Color  The color to draw the circle in.
        void DrawWorldRing(ConstRef<Camera> Camera, Vector3 Center, Vector3 AxisU, Vector3 AxisV, Real32 Radius, UInt32 Color);

        /// \brief Outlines the cone a spotlight covers, from its apex out to its range.
        ///
        /// \param Camera      The camera that projects world placements into the viewport.
        /// \param Transform The light's transform, whose X basis is the direction it aims along.
        /// \param Light     The light whose cone is outlined.
        void DrawSpotlightCone(ConstRef<Camera> Camera, ConstRef<Tileon::Transform> Transform, ConstRef<Tileon::Spotlight> Light);

        /// \brief Outlines the sphere a glowlight reaches into, at its scaled radius.
        ///
        /// \param Camera      The camera that projects world placements into the viewport.
        /// \param Transform The light's transform, whose scale sizes the reach.
        /// \param Light     The light whose reach is outlined.
        void DrawGlowlightArea(ConstRef<Camera> Camera, ConstRef<Tileon::Transform> Transform, ConstRef<Tileon::Glowlight> Light);

        /// \brief Drops a dotted stem from an elevated point to the ground it stands on.
        ///
        /// \param Camera  The camera that projects world placements into the viewport.
        /// \param World The elevated point, in absolute tiles.
        /// \param Color The color to draw the stem and its ground ring in.
        void DrawStem(ConstRef<Camera> Camera, Vector3 World, UInt32 Color);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Tools            mTools;
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