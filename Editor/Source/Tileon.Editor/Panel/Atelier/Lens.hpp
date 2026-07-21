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

#include "Tileon.Editor/Context.hpp"
#include "Tileon.Editor/UI/Composer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Maps between world placements and the viewport's screen-space rectangle.
    ///
    /// The overlay, gizmo, and brush previews all project the same way; this bundles the director with the
    /// viewport rect so each of them projects a point without re-deriving the pixel range.
    struct Lens final
    {
        /// The director whose camera defines the projection.
        Ref<Director> Director;

        /// The top-left corner of the viewport, in screen space.
        ImVec2        Origin;

        /// The size of the viewport, in screen space.
        ImVec2        Size;

        /// \brief Projects a world placement into the viewport.
        ///
        /// \param World The placement to project.
        /// \return The corresponding position in screen space.
        ZY_INLINE ImVec2 Project(Placement World) const
        {
            const Vector2 Pixel  = Director.GetScreenCoordinates(World);
            const Real32  RangeX = Director.GetViewport().GetX() * Director.GetDensity();
            const Real32  RangeY = Director.GetViewport().GetY() * Director.GetDensity();

            return ImVec2(
                Origin.x + (Pixel.GetX() / RangeX) * Size.x,
                Origin.y + (Pixel.GetY() / RangeY) * Size.y);
        }

        /// \brief Un-projects a viewport position back into the world.
        ///
        /// \param Point The position to un-project, in screen space.
        /// \return The corresponding world placement.
        ZY_INLINE Placement Unproject(ImVec2 Point) const
        {
            const Real32 RangeX = Director.GetViewport().GetX() * Director.GetDensity();
            const Real32 RangeY = Director.GetViewport().GetY() * Director.GetDensity();

            return Director.GetWorldCoordinates(
                Vector2(((Point.x - Origin.x) / Size.x) * RangeX, ((Point.y - Origin.y) / Size.y) * RangeY));
        }
    };
}
