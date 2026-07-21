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

#include "Tileon.Editor/UI/Composer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::UI
{
    /// \brief A modal ImGui file browser used in place of a native OS file dialog.
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
        ///
        /// \param Composer The UI composer used to render the picker.
        void Draw(Ref<Composer> Composer);

        /// \brief Checks whether the picker is currently open and awaiting user input.
        ///
        /// \return `true` if the picker is open, `false` otherwise.
        ZY_INLINE Bool IsOpen() const
        {
            return mOpen;
        }

    private:

        /// \brief Refreshes \ref mEntries from the current \ref mDirectory.
        void Refresh();

        /// \brief Navigates into a subdirectory or up to the parent, then refreshes the listing.
        ///
        /// \param Directory The directory to navigate to.
        void Navigate(Text Directory);

        /// \brief Confirms the current selection/filename and invokes the result callback.
        void Confirm();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Bool                         mOpen;
        Mode                         mMode;
        Filesystem::Path             mDirectory;
        Filesystem::Name             mExtension;
        Filesystem::Name             mFilename;
        SInt32                       mSelection;
        Sequence<Filesystem::Record> mEntries;
        OnResult                     mCallback;
    };
}