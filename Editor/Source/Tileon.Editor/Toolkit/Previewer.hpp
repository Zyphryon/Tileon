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

#include "Tileon.Editor/Toolkit/Composer.hpp"
#include "Tileon.Editor/Toolkit/Theme.hpp"
#include <Zyphryon.Math/Geometry/Rect.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Toolkit
{
    /// \brief A responsive widget that previews a texture, with zooming and panning.
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

        /// \brief Constructs a previewer with default zoom and pan settings.
        Previewer();

        /// \brief Draws the texture in the previewer with custom size.
        ///
        /// \param Texture The texture to be drawn in the previewer, which may name an array slice.
        /// \param Size    The size to draw the texture.
        /// \param Source  The source rectangle defining the portion of the texture to draw.
        /// \param Tint    The color tint to apply to the texture when drawing.
        void Draw(ImTextureID Texture, Vector2 Size, Rect Source, Color Tint = Color::White());

        /// \brief Gets the current zoom level for the previewer.
        ///
        /// \return The current zoom level.
        ZY_INLINE Real32 GetZoom() const
        {
            return mZoom;
        }

        /// \brief Sets the zoom level for the previewer.
        ///
        /// \param Zoom The zoom level to set.
        ZY_INLINE void SetZoom(Real32 Zoom)
        {
            mZoom = Clamp(Zoom, kZoomMin, kZoomMax);
        }

        /// \brief Sets the slice of an array texture the previewer is showing.
        ///
        /// \param Slice The zero-based slice within the array.
        ZY_INLINE void SetSlice(UInt16 Slice)
        {
            mSlice = Slice;
        }

        /// \brief Gets the slice of an array texture the previewer is showing.
        ///
        /// \return The slice currently shown.
        ZY_INLINE UInt16 GetSlice() const
        {
            return mSlice;
        }

        /// \brief Resets both the zoom level and pan offset to their default values.
        ZY_INLINE void Reset()
        {
            mZoom = 1.0f;
            mPan.Set(0.0f, 0.0f);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Real32  mZoom;
        Vector2 mPan;
        UInt16  mSlice;
    };
}