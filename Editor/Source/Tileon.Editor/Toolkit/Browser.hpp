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

#include "Tileon.Editor/Toolkit/Gallery.hpp"
#include <Zyphryon.Content/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Toolkit
{
    /// \brief Browses the content service for an asset, shared between any number of the fields that ask for one.
    class Browser final
    {
    public:

        /// \brief Defines the time interval in seconds for refreshing the content entries in the browser.
        static constexpr Real64 kInterval = 2.0f;

    public:

        /// \brief Constructs the browser widget with the specified content service.
        ///
        /// \param Service The content service used to access and manage the content items displayed in the browser.
        Browser(Ref<Content::Service> Service);

        /// \brief Opens the browser on behalf of the field identified by the given key.
        ///
        /// \param Key    The key identifying the requesting field.
        /// \param Filter The extensions the field takes, separated by spaces, or empty to take everything.
        void Open(UInt64 Key, Text Filter);

        /// \brief Draws the browser, which must be called once per frame at window scope.
        void Draw();

        /// \brief Claims the selection made on behalf of the field identified by the given key.
        ///
        /// \param Key       The key identifying the field claiming the selection.
        /// \param Selection The url of the selected asset, assigned only when a selection was pending.
        /// \return `true` if a selection was pending for the key, `false` otherwise.
        Bool Consume(UInt64 Key, Ref<Str> Selection);

        /// \brief Checks whether the browser is currently open on behalf of the given key.
        ///
        /// \param Key The key identifying the field to test.
        /// \return `true` if the field is awaiting a selection, `false` otherwise.
        ZY_INLINE Bool IsPending(UInt64 Key) const
        {
            return Key != 0 && mRequest == Key;
        }

    private:

        /// \brief Defines the type for a list of content entries.
        using Entries = Sequence<Filesystem::Record>;

        /// \brief A cached directory listing together with its asynchronous refresh bookkeeping.
        struct Directory
        {
            /// The most recently enumerated entries for the directory.
            Entries Records;

            /// Whether an enumeration request for this directory is currently in flight.
            Bool    Pending = false;

            /// The earliest time, in seconds, at which a new enumeration may be issued.
            Real64  Refresh = 0.0;
        };

        /// \brief Checks whether an entry is one of the kinds the open field asked for.
        ///
        /// \param Name The file name to test.
        /// \return `true` when the field takes the entry, `false` otherwise.
        Bool Accepts(Text Name) const;

        /// \brief Draws the browser as a popup window, providing a dedicated interface for browsing content items.
        ///
        /// \return `true` if the popup is still open, `false` if it was closed.
        Bool DrawPopup();

        /// \brief Resets the browser state, clearing any cached entries and resetting the current path and selection.
        void Reset();

        /// \brief Draws the main body of the browser, which includes the sidebar tree view and the content gallery.
        void DrawBody();

        /// \brief Recursively draws one level of the directory tree for the given content URI.
        ///
        /// \param Parent The content URI of the directory to draw, whose children will be enumerated.
        void DrawSidebarTree(ConstRef<Content::Uri> Parent);

        /// \brief Draws the right content area using the gallery widget.
        void DrawContent();

        /// \brief Returns a snapshot of the cached entries for the given URI.
        ///
        /// \param Uri The content URI of the directory to enumerate.
        /// \return A copy of the currently cached entries, which may be empty while a request is in flight.
        Entries GetEntries(ConstRef<Content::Uri> Uri);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Content::Service>    mService;
        Mutex                    mMutex;
        Table<UInt64, Directory> mEntries;
        Content::Uri             mPath;
        Gallery                  mItems;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Bool                     mOpen;
        Str                      mSelection;
        Sequence<Str>            mFilters;
        UInt64                   mRequest;
        UInt64                   mResult;
        Str                      mAnswer;
    };
}