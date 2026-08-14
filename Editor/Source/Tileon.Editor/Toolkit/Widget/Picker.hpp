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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Toolkit
{
    /// \brief A modal file browser used in place of a native OS file dialog.
    class Picker final
    {
    public:

        /// \brief Determines whether the picker is browsing for a file to open or a destination to save.
        enum class Mode : UInt8
        {
            Open,   ///< Selecting an existing file to open.
            Save,   ///< Choosing a destination filename to save to.
        };

        /// \brief Callback invoked when the user confirms a file selection.
        ///
        /// \param Path The absolute path of the file chosen by the user.
        using OnResult = Delegate<void(Text Path)>;

    public:

        /// \brief Constructs a picker with default state.
        Picker();

        /// \brief Opens the picker, resetting it to browse the given starting directory.
        ///
        /// \param Mode      Whether the picker is used to open or save a file.
        /// \param Directory The directory to start browsing from.
        /// \param Extension The required file extension filter (e.g. ".tileon"), or empty for no filter.
        /// \param Callback  The callback invoked with the selected path once the user confirms.
        void Open(Mode Mode, Text Directory, Text Extension, AnyRef<OnResult> Callback);

        /// \brief Draws the picker if currently open. Must be called every frame while active.
        void Draw();

        /// \brief Checks whether the picker is currently open and awaiting user input.
        ///
        /// \return `true` if the picker is open, `false` otherwise.
        ZY_INLINE Bool IsOpen() const
        {
            return mOpen;
        }

    private:

        /// \brief Identifies the listing column the entries are ordered by, matching the table's column indices.
        enum class Column : UInt8
        {
            Name,   ///< Ordered by entry name.
            Size,   ///< Ordered by file size.
            Type,   ///< Ordered by the described entry type.
        };

        /// \brief Refreshes \ref mEntries from the current \ref mDirectory.
        void Refresh();

        /// \brief Orders \ref mEntries by the active column, keeping directories ahead of files either way.
        void Sort();

        /// \brief Browses to a directory and records it, discarding any trail the user had moved back from.
        ///
        /// \param Directory The directory to navigate to.
        void Navigate(Text Directory);

        /// \brief Steps through the visited directories without disturbing the trail.
        ///
        /// \param Steps How many entries to move, negative to go back and positive to go forward.
        void Travel(SInt32 Steps);

        /// \brief Browses to the parent of the directory currently shown.
        void Ascend();

        /// \brief Browses to a path typed into the address bar, which may name a directory or a file.
        ///
        /// \param Address The path as typed, in either separator style and with or without a trailing one.
        /// \return `true` if the path resolved and the picker moved there, otherwise `false`.
        Bool Locate(Text Address);

        /// \brief Confirms the current filename and invokes the result callback.
        ///
        /// \return `true` if a filename was present and the callback ran, otherwise `false`.
        Bool Confirm();

        /// \brief Draws the history buttons, the address bar and the search box.
        void DrawToolbar();

        /// \brief Draws the breadcrumb trail, shedding leading crumbs that do not fit the given width.
        ///
        /// \param Width The horizontal room the trail has to lay itself out in.
        void DrawTrail(Real32 Width);

        /// \brief Draws the sortable listing of directories and files.
        ///
        /// \return `true` if an entry was double clicked to confirm the dialog.
        Bool DrawListing();

        /// \brief Draws the filename row, the type filter and the confirm / cancel buttons.
        ///
        /// \return `true` if the dialog was confirmed or cancelled and its popup should close.
        Bool DrawFooter();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Bool                         mOpen;
        Mode                         mMode;
        Column                       mColumn;
        Bool                         mAscending;
        Bool                         mEditing;
        Filesystem::Path             mAddress;
        Filesystem::Path             mDirectory;
        Filesystem::Name             mExtension;
        Filesystem::Name             mFilename;
        Filesystem::Name             mSearch;
        Sequence<Filesystem::Record> mEntries;
        Sequence<Filesystem::Path>   mHistory;
        SInt32                       mCursor;
        OnResult                     mCallback;
    };
}
