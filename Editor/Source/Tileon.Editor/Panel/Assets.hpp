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

#include "Tileon.Editor/Asset/AssetCatalog.hpp"
#include "Tileon.Editor/Asset/AssetEditor.hpp"
#include "Tileon.Editor/Asset/Importer.hpp"
#include "Tileon.Editor/Panel.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Presents the project's assets as a folder tree and the files inside the folder in view.
    class Assets final : public Panel
    {
    public:

        /// \brief Constructs the activity with the specified context.
        ///
        /// \param Context The context associated with this activity.
        Assets(Ref<Context> Context);

        /// \see Panel::OnDraw()
        void OnDraw() override;

    private:

        /// \brief Enumerates the keys the listing can be ordered by, in column order.
        enum class Column : UInt8
        {
            Name, ///< Ordered by file name, without regard to case.
            Size, ///< Ordered by size on disk.
        };

        /// \brief Describes one entry of the folder in view.
        struct Entry final
        {
            /// The entry's file name, without its folder.
            Str                 Name;

            /// The kind the entry belongs to, or `nullptr` for a folder or a file nothing knows about.
            ConstPtr<AssetType> Type;

            /// `true` when the entry is a directory.
            Bool                Folder;

            /// The entry's size on disk, in bytes.
            UInt64              Size;

            /// \brief Constructs an entry standing for nothing.
            ZY_INLINE Entry()
                : Type   { nullptr },
                  Folder { false },
                  Size   { 0 }
            {
            }

            /// \brief Constructs an entry with the specified name, kind and size.
            ///
            /// \param Name   The entry's file name, without its folder.
            /// \param Type   The kind the entry belongs to, or `nullptr` for a folder or an unknown file.
            /// \param Folder `true` when the entry is a directory.
            /// \param Size   The entry's size on disk, in bytes.
            ZY_INLINE Entry(AnyRef<Str> Name, ConstPtr<AssetType> Type, Bool Folder, UInt64 Size)
                : Name   { Move(Name) },
                  Type   { Type },
                  Folder { Folder },
                  Size   { Size }
            {
            }
        };

        /// \brief Gets the glyph an entry is listed with.
        ///
        /// \param Item The entry the row stands for.
        /// \return The icon for the entry.
        static Text GetIcon(ConstRef<Entry> Item);

        /// \brief Rereads the folder in view, which is what every action that touches disk ends with.
        void Refresh();

        /// \brief Reorders the folder in view by whichever column the header row asks for.
        void Sort();

        /// \brief Opens the specified folder, relative to the project's root.
        ///
        /// \param Folder The folder to open, which may be empty for the root itself.
        void Navigate(AnyRef<Str> Folder);

        /// \brief Draws the trail of folders leading to the one in view, each of them a way back.
        void DrawTrail();

        /// \brief Draws the folder in view as a table of its entries.
        void DrawEntries();

        /// \brief Gets the editor that authors a kind of asset.
        ///
        /// \param Type The kind to look up.
        /// \return The editor for the kind, or `nullptr` when nothing authors it.
        Ptr<AssetEditor> Reach(ConstRef<AssetType> Type) const;

        /// \brief Draws the menu an entry opens on right click.
        ///
        /// \param Item The entry the row stands for.
        void DrawEntryMenu(ConstRef<Entry> Item);

        /// \brief Draws the menu the folder in view opens on right click, which is where creating happens.
        void DrawFolderMenu();

        /// \brief Draws the prompt that names a folder before it is created.
        void DrawFolderPrompt();

        /// \brief Draws the prompt that stands between an entry and its removal from disk.
        void DrawDeletePrompt();

        /// \brief Drops from memory whatever an entry holds, so nothing outlives the file it was read from.
        ///
        /// \param Name   The entry to drop, relative to the folder in view.
        /// \param Folder `true` when the entry is a directory, whose contents are dropped instead.
        void Evict(Text Name, Bool Folder);

        /// \brief Gets the absolute path of a name inside the folder in view.
        ///
        /// \param Name The file name to resolve, which may be empty for the folder itself.
        /// \return The path on disk.
        Str Resolve(Text Name);

        /// \brief Gets the url a name inside the folder in view is loaded under.
        ///
        /// \param Name The file name to locate.
        /// \return The url the content service knows the asset by.
        Content::Uri Locate(Text Name);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Str                           mFolder;
        Sequence<Entry>               mEntries;
        Str                           mSelection;
        ConstPtr<AssetType>           mCreating;
        Bool                          mCreatingFolder;
        Str                           mCreation;
        Str                           mDeleting;
        Bool                          mDeletingFolder;
        Column                        mColumn;
        Bool                          mAscending;
        Str                           mHovered;
        UInt32                        mStep;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        AssetCatalog                  mCatalog;
        Importer                      mImporter;
        Sequence<Unique<AssetEditor>> mEditors;
    };
}