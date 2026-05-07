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
#include <Zyphryon.Graphic/Texture.hpp>
#include <Zyphryon.Math/Geometry/Rect.hpp>
#include <Zyphryon.Math/Color.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::UI
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
        ///
        /// \param Composer The UI composer used to render the gallery elements.
        void DrawToolbar(Ref<Composer> Composer);

        /// \brief Begins the gallery section, setting up necessary state and layout for rendering items.
        ///
        /// \param Composer The UI composer used to render the gallery elements.
        void Begin(Ref<Composer> Composer);

        /// \brief Draws an individual item in the gallery, displaying its thumbnail and name, and handling selection logic.
        ///
        /// \param Composer  The UI composer used to render the gallery elements.
        /// \param ID        The unique identifier for the item being drawn.
        /// \param Name      The display name of the item, shown alongside the thumbnail in the gallery.
        /// \param Thumbnail The graphic object representing the thumbnail image for the item.
        /// \param Crop      The rectangular region of the thumbnail to display.
        /// \param Tint      The tint color to apply to the thumbnail when rendering.
        /// \return `true` if the item was selected, `false` otherwise.
        Bool DrawItem(Ref<Composer> Composer, UInt32 ID, ConstStr8 Name, Graphic::Object Thumbnail, Rect Crop, IntColor8 Tint);

        /// \brief Overload of `DrawItem` that allows drawing an item without a thumbnail.
        ///
        /// \param Composer The UI composer used to render the gallery elements.
        /// \param ID       The unique identifier for the item being drawn.
        /// \param Name     The display name of the item, shown in the gallery.
        /// \return `true` if the item was selected, `false` otherwise.
        ZYPHRYON_INLINE Bool DrawItem(Ref<Composer> Composer, UInt32 ID, ConstStr8 Name)
        {
            return DrawItem(Composer, ID, Name, 0, Rect::One(), IntColor8::White());
        }

        /// \brief Ends the gallery section, finalizing any state or layout changes made during the gallery rendering.
        ///
        /// \param Composer The UI composer used to render the gallery elements.
        void End(Ref<Composer> Composer);

        /// \brief Sets the display mode for the gallery.
        ///
        /// \param Mode The display mode to set for the gallery.
        ZYPHRYON_INLINE void SetMode(Mode Mode)
        {
            mMode = Mode;
        }

        /// \brief Gets the current display mode of the gallery.
        ///
        /// \return The current display mode of the gallery.
        ZYPHRYON_INLINE Mode GetMode() const
        {
            return mMode;
        }

        /// \brief Sets the size of the thumbnails in the gallery.
        ///
        /// \param Size The new size to set for the thumbnails in the gallery, in pixels.
        ZYPHRYON_INLINE void SetSize(Real32 Size)
        {
            mSize = Clamp(Size, kThumbnailMinSize, kThumbnailMaxSize);
        }

        /// \brief Gets the current size of the thumbnails in the gallery.
        ///
        /// \return The current size of the thumbnails in the gallery, in pixels.
        ZYPHRYON_INLINE Real32 GetSize() const
        {
            return mSize;
        }

        /// \brief Sets the filter string used to filter items in the gallery based on their names.
        ///
        /// \param Filter The new filter string to set for the gallery.
        ZYPHRYON_INLINE void SetFilter(ConstStr8 Filter)
        {
            mFilter = Filter;
        }

        /// \brief Gets the current filter string used to filter items in the gallery based on their names.
        ///
        /// \return The current filter string used to filter items in the gallery.
        ZYPHRYON_INLINE ConstStr8 GetFilter() const
        {
            return mFilter;
        }

        /// \brief Sets the currently selected item in the gallery by its unique identifier.
        ///
        /// \param Selection The unique identifier of the item to set as selected in the gallery.
        ZYPHRYON_INLINE void SetSelection(UInt32 Selection)
        {
            mSelection = Selection;
        }

        /// \brief Gets the unique identifier of the currently selected item in the gallery.
        ///
        /// \return The unique identifier of the currently selected item in the gallery.
        ZYPHRYON_INLINE UInt32 GetSelection() const
        {
            return mSelection;
        }

    private:

        /// \brief Checks if an item with the specified name matches the current filter string.
        ///
        /// \param Name The name of the item to check against the current filter string.
        /// \return `true` if the item matches the filter and should be displayed in the gallery, `false` otherwise.
        Bool Filter(ConstStr8 Name) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Bool    mActive;
        Mode    mMode;
        Real32  mSize;
        Str8    mFilter;
        UInt32  mSelection;
    };
}

