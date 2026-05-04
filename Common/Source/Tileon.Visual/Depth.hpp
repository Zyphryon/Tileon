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

#include <Zyphryon.Math/Geometry/Rect.hpp>
#include <Zyphryon.Math/Vector2.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Visual
{
    /// \brief Provides depth-value utilities for the 2D rendering pipeline.
    class Depth final
    {
    public:

        /// \brief The depth value for the foreground layer.
        static constexpr Real32 kForeground = 0.00f;

        /// \brief The depth value for the midground layer.
        static constexpr Real32 kMidground  = 0.10f;

        /// \brief The depth value for the background layer.
        static constexpr Real32 kBackground = 0.90f;

    public:

        /// \brief Calculates the depth value for the foreground layer.
        ///
        /// \param Bias An optional bias value to slightly adjust the depth, allowing for fine-tuning of rendering order.
        /// \return The calculated depth value for the foreground layer.
        ZYPHRYON_INLINE static constexpr Real32 Foreground(UInt8 Bias = 0)
        {
            return kForeground + Bias * 0.01f;
        }

        /// \brief Calculates the depth value for the background layer.
        ///
        /// \param Bias An optional bias value to slightly adjust the depth, allowing for fine-tuning of rendering order.
        /// \return The calculated depth value for the background layer.
        ZYPHRYON_INLINE static constexpr Real32 Background(UInt8 Bias = 0)
        {
            return 1.0f - Bias * 0.01f;
        }

        /// \brief Calculates the depth value for the midground layer based on the position within the frustum.
        ///
        /// \param Frustum The rectangular area representing the current view frustum.
        /// \param LocalX  The local x-coordinate within the frustum.
        /// \param LocalY  The local y-coordinate within the frustum.
        /// \param Bias    An optional bias value to slightly adjust the depth, allowing for fine tuning of rendering order.
        /// \return The calculated depth value for the midground layer.
        ZYPHRYON_INLINE static constexpr Real32 Midground(IntRect Frustum, Real32 LocalX, Real32 LocalY, UInt8 Bias = 0)
        {
            constexpr Real32 kRange = kBackground - kMidground;

            const Real32 ClampedX = Clamp(LocalX, 0.0f, static_cast<Real32>(Frustum.GetWidth()));
            const Real32 ClampedY = Clamp(LocalY, 0.0f, static_cast<Real32>(Frustum.GetHeight()));
            const Real32 NormX    = 1.0f - ClampedX / static_cast<Real32>(Frustum.GetWidth());
            const Real32 NormY    =        ClampedY / static_cast<Real32>(Frustum.GetHeight());

            return kMidground + NormY * kRange * 0.999f + NormX * kRange * 0.0009f + Bias * 0.00001f;
        }

        /// \brief Calculates the depth value for the midground layer based on the position within the frustum.
        ///
        /// \param Frustum The rectangular area representing the current view frustum.
        /// \param Shift   The integer shift representing the position within the frustum, typically in tile coordinates.
        /// \param Offset  The offset to apply to the shift, allowing for sub-tile adjustments.
        /// \param Bias    An optional bias value to slightly adjust the depth, allowing for fine tuning of rendering order.
        /// \return The calculated depth value for the midground layer.
        ZYPHRYON_INLINE static constexpr Real32 Midground(IntRect Frustum, IntVector2 Shift, Vector2 Offset, UInt8 Bias = 0)
        {
            const Real32 LocalX = static_cast<Real32>(Shift.GetX() - Frustum.GetMinimumX()) + Offset.GetX();
            const Real32 LocalY = static_cast<Real32>(Shift.GetY() - Frustum.GetMinimumY()) + Offset.GetY();
            return Midground(Frustum, LocalX, LocalY, Bias);
        }
    };
}

