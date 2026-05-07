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

#include "Gallery.hpp"
#include <Zyphryon.Content/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::UI
{
    /// \brief Provides a browser widget for navigating and selecting content items from the content service.
    class Browser final
    {
    public:

        /// \brief Defines the time interval in seconds for refreshing the content entries in the browser.
        static constexpr Real64 kInterval = 2.0f;

        /// \brief The different display modes for the browser, determining how the content is presented to the user.
        enum class Mode
        {
            Inline, ///< The browser is displayed inline within the current UI context.
            Popup,  ///< The browser is displayed as a popup window, providing a dedicated interface for browsing.
        };

    public:

        /// \brief Constructs the browser widget with the specified content service.
        ///
        /// \param Service The content service used to access and manage the content items displayed in the browser.
        /// \param Mode    The display mode for the browser, determining how it will be rendered in the UI.
        Browser(Ref<Content::Service> Service, Mode Mode);

        /// \brief Opens the browser, setting its state to open and allowing it to be drawn in the UI.
        ///
        /// \param Filter An optional filter to limit the displayed items based on their types.
        void Open(ConstStr8 Filter = "");

        /// \brief Draws the browser widget, which consists of a sidebar tree view and a main gallery view.
        ///
        /// \param Composer The UI composer used to render the browser interface elements.
        /// \return `true` if the popup is still open, `false` if it was closed.
        Bool Draw(Ref<UI::Composer> Composer);

        /// \brief Resets the browser state, clearing any cached entries and resetting the current path and selection.
        void Reset();

        /// \brief Gets the currently selected content URI in the browser.
        ///
        /// \return The content URI of the currently selected item in the browser.
        ZYPHRYON_INLINE ConstStr8 GetSelection() const
        {
            return  mSelection;
        }

    private:

        /// \brief Draws the browser as a popup window, providing a dedicated interface for browsing content items.
        ///
        /// \param Composer The UI composer used to render the popup window and its contents.
        /// \return `true` if the popup is still open, `false` if it was closed.
        Bool DrawPopup(Ref<UI::Composer> Composer);

        /// \brief Draws the main body of the browser, which includes the sidebar tree view and the content gallery.
        ///
        /// \param Composer The UI composer used to render the main body of the browser,.
        void DrawBody(Ref<UI::Composer> Composer);

        /// \brief Recursively draws one level of the directory tree for the given content URI.
        ///
        /// \param Composer The UI composer used to render the tree nodes.
        /// \param Parent   The content URI of the directory to draw, whose children will be enumerated.
        void DrawSidebarTree(Ref<UI::Composer> Composer, ConstRef<Content::Uri> Parent);

        /// \brief Draws the right content area using the gallery widget.
        ///
        /// \param Composer The UI composer used to render the gallery.
        void DrawContent(Ref<UI::Composer> Composer);

        /// \brief Defines the type for a list of content entries.
        using Entries = Vector<Content::Mount::Item>;

        /// \brief Returns the cached entry list for the given URI, populating the cache on first access.
        ///
        /// \param Uri The content URI of the directory to enumerate.
        ConstRef<Entries> GetEntries(ConstRef<Content::Uri> Uri);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Content::Service>  mService;
        Table<UInt64, Entries> mEntries;        // TODO: Use LastTimeUpdated to reduce queries.
        Content::Uri           mPath;
        UI::Gallery            mGallery;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Mode                   mMode;
        Bool                   mOpen;
        Real64                 mTime;
        Str8                   mSelection;
        Str8                   mFilter;
    };
}