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
#include "Tileon.Editor/Toolkit/Composer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Maps between world placements and the viewport's screen-space rectangle.
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

        /// \brief Projects a world placement into the viewport at the given height above the ground.
        ///
        /// \param World     The ground placement to project.
        /// \param Elevation The height above the ground plane.
        /// \return The corresponding position in screen space.
        ZY_INLINE ImVec2 Project(Placement World, Real32 Elevation) const
        {
            const ImVec2 Ground = Project(World);
            const ImVec2 Lift   = Direction(Vector3(0.0f, Elevation, 0.0f));

            return ImVec2(Ground.x + Lift.x, Ground.y + Lift.y);
        }

        /// \brief Projects a world-space offset into the viewport's own scale.
        ///
        /// \param World The world-space offset to project.
        /// \return The corresponding offset in screen space.
        ZY_INLINE ImVec2 Direction(Vector3 World) const
        {
            const Vector2 Pixel  = Director.GetScreenDirection(World);
            const Real32  RangeX = Director.GetViewport().GetX() * Director.GetDensity();
            const Real32  RangeY = Director.GetViewport().GetY() * Director.GetDensity();

            return ImVec2((Pixel.GetX() / RangeX) * Size.x, (Pixel.GetY() / RangeY) * Size.y);
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