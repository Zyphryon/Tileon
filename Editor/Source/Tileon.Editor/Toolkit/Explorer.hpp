// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Toolkit
{
    /// \brief A modal file explorer used in place of the native OS file dialog.
    class Explorer final
    {
    public:

        /// \brief Determines whether the explorer is browsing for a file to open or a destination to save.
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

        /// \brief One named group of suffixes the explorer offers, as a file dialog's filter box does.
        struct Filter final
        {
            /// \brief Constructs a group naming nothing.
            ZY_INLINE Filter() = default;

            /// \brief Constructs a group with the specified label and suffixes.
            ///
            /// \param Label   What the group is called, such as "Image".
            /// \param Sources The suffixes it accepts, separated by spaces.
            ZY_INLINE Filter(Text Label, Text Sources)
                : Label   { Label },
                  Sources { Sources }
            {
            }

            /// What the group is called, which may be empty when there is nothing to choose between.
            Str Label;

            /// The suffixes the group accepts, separated by spaces.
            Str Sources;
        };

    public:

        /// \brief Constructs an explorer with default state.
        Explorer();

        /// \brief Opens the explorer, offering a choice of the kinds of file it lists.
        ///
        /// \param Mode      Whether the explorer is used to open or save a file.
        /// \param Directory The directory to start browsing from.
        /// \param Filters   The groups the filter box offers, the first of which is selected.
        /// \param Callback  The callback invoked with the selected path once the user confirms.
        void Open(Mode Mode, Text Directory, ConstSpan<Filter> Filters, AnyRef<OnResult> Callback);

        /// \brief Opens the explorer, resetting it to browse the given starting directory.
        ///
        /// \param Mode      Whether the explorer is used to open or save a file.
        /// \param Directory The directory to start browsing from.
        /// \param Extension The suffixes to accept (e.g. ".tileon"), or empty for no filter.
        /// \param Callback  The callback invoked with the selected path once the user confirms.
        void Open(Mode Mode, Text Directory, Text Extension, AnyRef<OnResult> Callback);

        /// \brief Gets the text the filter box shows for a group.
        ///
        /// \param Index The group to describe.
        /// \return The label and its suffixes, or "All Files" when the group accepts everything.
        String<96> Describe(UInt32 Index) const;

        /// \brief Draws the explorer if currently open. Must be called every frame while active.
        void Draw();

        /// \brief Checks whether the explorer is currently open and awaiting user input.
        ///
        /// \return `true` if the explorer is open, `false` otherwise.
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
        /// \return `true` if the path resolved and the explorer moved there, otherwise `false`.
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
        /// \return `true` if an entry was double clicked to confirm the selection.
        Bool DrawListing();

        /// \brief Draws the filename row, the type filter and the confirm / cancel buttons.
        ///
        /// \return `true` if the explorer was confirmed or cancelled and its popup should close.
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
        Sequence<Filter>             mFilters;
        UInt32                       mSelected;
        Filesystem::Name             mFilename;
        Filesystem::Name             mSearch;
        Sequence<Filesystem::Record> mEntries;
        Sequence<Filesystem::Path>   mHistory;
        SInt32                       mCursor;
        OnResult                     mCallback;
    };
}