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
#include <Zyphryon.Graphic/Image.hpp>
#include <Zyphryon.Math/Geometry/Rect.hpp>
#include <Zyphryon.Math/Color.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Toolkit
{
    /// \brief A reusable widget that displays a browsable collection of items in either List or Grid mode.
    class Gallery final
    {
    public:

        /// \brief The minimum allowed size for thumbnails in the gallery.
        static constexpr Real32 kThumbnailMinSize = 16.0f;

        /// \brief The maximum allowed size for thumbnails in the gallery.
        static constexpr Real32 kThumbnailMaxSize = 128.0f;

        /// \brief The different display modes for the gallery, determining how items are arranged and rendered.
        enum class Mode : UInt8
        {
            List,   ///< Displays items in a vertical list, showing one item per row with a thumbnail and name.
            Grid,   ///< Displays items in a grid layout, showing multiple items per row with thumbnails and names.
        };

    public:

        /// \brief Constructs a gallery with default settings.
        Gallery();

        /// \brief Optionally draw a toolbar for the gallery, allowing to switch between display modes.
        void DrawToolbar();

        /// \brief Begins the gallery section, setting up necessary state and layout for rendering items.
        void Begin();

        /// \brief Draws an individual item in the gallery, displaying its thumbnail and name, and handling selection logic.
        ///
        /// \param ID        The unique identifier for the item being drawn.
        /// \param Name      The display name of the item, shown alongside the thumbnail in the gallery.
        /// \param Thumbnail The thumbnail image for the item, which may name an array slice.
        /// \param Crop      The rectangular region of the thumbnail to display.
        /// \param Tint      The tint color to apply to the thumbnail when rendering.
        /// \return `true` if the item was selected, `false` otherwise.
        Bool DrawItem(UInt32 ID, Text Name, ImTextureID Thumbnail, Rect Crop, IntColor8 Tint);

        /// \brief Overload of `DrawItem` that allows drawing an item without a thumbnail.
        ///
        /// \param ID   The unique identifier for the item being drawn.
        /// \param Name The display name of the item, shown in the gallery.
        /// \return `true` if the item was selected, `false` otherwise.
        ZY_INLINE Bool DrawItem(UInt32 ID, Text Name)
        {
            return DrawItem(ID, Name, 0, Rect::One(), IntColor8::White());
        }

        /// \brief Ends the gallery section, finalizing any state or layout changes made during the gallery rendering.
        void End();

        /// \brief Sets the display mode for the gallery.
        ///
        /// \param Mode The display mode to set for the gallery.
        ZY_INLINE void SetMode(Mode Mode)
        {
            mMode = Mode;
        }

        /// \brief Gets the current display mode of the gallery.
        ///
        /// \return The current display mode of the gallery.
        ZY_INLINE Mode GetMode() const
        {
            return mMode;
        }

        /// \brief Sets the size of the thumbnails in the gallery.
        ///
        /// \param Size The new size to set for the thumbnails in the gallery, in pixels.
        ZY_INLINE void SetSize(Real32 Size)
        {
            mSize = Clamp(Size, kThumbnailMinSize, kThumbnailMaxSize);
        }

        /// \brief Gets the current size of the thumbnails in the gallery.
        ///
        /// \return The current size of the thumbnails in the gallery, in pixels.
        ZY_INLINE Real32 GetSize() const
        {
            return mSize;
        }

        /// \brief Sets the currently selected item in the gallery by its unique identifier.
        ///
        /// \param Selection The unique identifier of the item to set as selected in the gallery.
        ZY_INLINE void SetSelection(UInt32 Selection)
        {
            mSelection = Selection;
        }

        /// \brief Gets the unique identifier of the currently selected item in the gallery.
        ///
        /// \return The unique identifier of the currently selected item in the gallery.
        ZY_INLINE UInt32 GetSelection() const
        {
            return mSelection;
        }

        /// \brief Gets the item the user asked to open this frame by right-clicking it.
        ///
        /// The value is reset every \ref Begin, so callers read it once after \ref End.
        ///
        /// \return The identifier of the right-clicked item, or `-1` if none was activated this frame.
        ZY_INLINE SInt64 GetActivated() const
        {
            return mActivated;
        }

    private:

        /// \brief One grid cell, held until the whole grid can be drawn a primitive at a time.
        struct Cell final
        {
            /// \brief Constructs a cell standing for nothing.
            ZY_INLINE Cell()
                : Minimum { 0.0f, 0.0f },
                  Maximum { 0.0f, 0.0f },
                  First   { 0.0f, 0.0f },
                  Last    { 0.0f, 0.0f },
                  Texture { 0 },
                  Tint    { 0 },
                  Fill    { 0 },
                  Border  { 0 }
            {
            }

            /// The upper left corner of the cell, in screen space.
            ImVec2      Minimum;

            /// The lower right corner of the cell, in screen space.
            ImVec2      Maximum;

            /// The upper left texture coordinates of the thumbnail.
            ImVec2      First;

            /// The lower right texture coordinates of the thumbnail.
            ImVec2      Last;

            /// The thumbnail the cell shows, or zero when it draws a placeholder instead.
            ImTextureID Texture;

            /// The tint applied to the thumbnail.
            ImU32       Tint;

            /// The highlight behind the thumbnail, or zero when the cell is neither selected nor hovered.
            ImU32       Fill;

            /// The colour of the cell's outline.
            ImU32       Border;
        };

        /// \brief Draws every cell the grid gathered, one primitive at a time.
        void Flush();

        /// \brief Checks if an item with the specified name matches the current filter string.
        ///
        /// \param Name The name of the item to check against the current filter string.
        /// \return `true` if the item matches the filter and should be displayed in the gallery, `false` otherwise.
        Bool Filter(Text Name) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Bool           mActive;
        Mode           mMode;
        Real32         mSize;
        Str            mFilter;
        UInt32         mSelection;
        SInt64         mActivated;
        Sequence<Cell> mCells;
    };
}