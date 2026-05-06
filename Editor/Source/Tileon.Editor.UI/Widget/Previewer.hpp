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

#include "Tileon.Editor.UI/Composer.hpp"
#include <Zyphryon.Math/Geometry/Rect.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::UI
{
    /// \brief A responsive image viewer widget that displays a texture with support for zooming and panning.
    class Previewer final
    {
    public:

        /// \brief Minimum allowed zoom level (25 %).
        static constexpr Real32 kZoomMin  = 0.25f;

        /// \brief Maximum allowed zoom level (800 %).
        static constexpr Real32 kZoomMax  = 8.00f;

        /// \brief Multiplicative step applied per scroll-wheel tick.
        static constexpr Real32 kZoomStep = 1.10f;

    public:

        /// \brief Constructs a previewer instance with default zoom and pan settings.
        Previewer();

        /// \brief Draws the texture in the previewer with custom size.
        ///
        /// \param Composer The UI composer used to render the texture.
        /// \param Texture  The texture to be drawn in the previewer.
        /// \param Size     The size to draw the texture.
        /// \param Source   The source rectangle defining the portion of the texture to draw.
        /// \param Tint     The color tint to apply to the texture when drawing.
        void Draw(Ref<Composer> Composer, Graphic::Object Texture, Vector2 Size, Rect Source, Color Tint = Color::White());

        /// \brief Gets the current zoom level for the previewer.
        ///
        /// \return The current zoom level.
        ZYPHRYON_INLINE Real32 GetZoom() const
        {
            return mZoom;
        }

        /// \brief Sets the zoom level for the previewer.
        ///
        /// \param Zoom The zoom level to set.
        ZYPHRYON_INLINE void SetZoom(Real32 Zoom)
        {
            mZoom = Math::Clamp(Zoom, kZoomMin, kZoomMax);
        }

        /// \brief Gets the pan offset for the previewer.
        ///
        /// \return The current pan offset.
        ZYPHRYON_INLINE Vector2 GetPan() const
        {
            return mPan;
        }

        /// \brief Sets the pan offset for the previewer.
        ///
        /// \param Pan The pan offset to set, where (0, 0) .
        ZYPHRYON_INLINE void SetPan(Vector2 Pan)
        {
            mPan = Pan;
        }

        /// \brief Resets the zoom level to its default value (1.0).
        ZYPHRYON_INLINE void ResetZoom()
        {
            mZoom = 1.0f;
        }

        /// \brief Resets the pan offset to its default value (0, 0).
        ZYPHRYON_INLINE void ResetPan()
        {
            mPan.Set(0.0f, 0.0f);
        }

        /// \brief Resets both the zoom level and pan offset to their default values.
        ZYPHRYON_INLINE void Reset()
        {
            ResetZoom();
            ResetPan();
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Real32  mZoom;
        Vector2 mPan;
    };
}
