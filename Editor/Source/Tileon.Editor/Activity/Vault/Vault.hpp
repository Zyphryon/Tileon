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

#include "Importer.hpp"
#include "Glaze.hpp"
#include "Tileon.Editor/Activity.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Presents the project's resources as a folder tree and the files inside the folder in view.
    class Vault final : public Activity
    {
    public:

        /// \brief Constructs the activity with the specified context.
        ///
        /// \param Context The context associated with this activity.
        Vault(Ref<Context> Context);

        /// \see Activity::OnDraw()
        void OnDraw() override;

    private:

        /// \brief Enumerates the kinds of resource the vault knows how to act on.
        enum class Kind : UInt8
        {
            Folder,     ///< A directory, which can be opened and imported into.
            Material,   ///< A material, whose images and parameters are authored.
            Font,       ///< A baked font.
            Technique,  ///< A shader technique.
            Texture,    ///< A baked texture.
            Source,     ///< Art or a typeface sitting in the project, which nothing baked loads.
            Other,      ///< Anything the editor has nothing to say about.
        };

        /// \brief Describes one entry of the folder in view.
        struct Entry final
        {
            /// \brief Constructs an entry standing for nothing.
            ZY_INLINE Entry()
                : Kind { Kind::Other },
                  Size { 0 }
            {
            }

            /// \brief Constructs an entry with the specified name, kind and size.
            ///
            /// \param Name The entry's file name, without its folder.
            /// \param Kind What the editor takes the entry to be.
            /// \param Size The entry's size on disk, in bytes.
            ZY_INLINE Entry(AnyRef<Str> Name, Vault::Kind Kind, UInt64 Size)
                : Name { Move(Name) },
                  Kind { Kind },
                  Size { Size }
            {
            }

            /// The entry's file name, without its folder.
            Str         Name;

            /// What the editor takes the entry to be.
            Vault::Kind Kind;

            /// The entry's size on disk, in bytes.
            UInt64      Size;
        };

        /// \brief Gets what the editor takes a file to be, from its extension.
        ///
        /// \param Name The file name to classify.
        /// \return The kind the file is treated as.
        static Kind Classify(Text Name);

        /// \brief Gets the icon a kind is listed with.
        ///
        /// \param Kind The kind to look up.
        /// \return The icon for the kind.
        static Text GetIcon(Kind Kind);

        /// \brief Rereads the folder in view, which is what every action that touches disk ends with.
        void Refresh();

        /// \brief Opens the specified folder, relative to the project's root.
        ///
        /// \param Folder The folder to open, which may be empty for the root itself.
        void Navigate(AnyRef<Str> Folder);

        /// \brief Draws the trail of folders leading to the one in view, each of them a way back.
        void DrawTrail();

        /// \brief Draws the folder in view as a table of its entries.
        void DrawEntries();

        /// \brief Draws the menu an entry opens on right click.
        ///
        /// \param Item The entry the row stands for.
        void DrawEntryMenu(ConstRef<Entry> Item);

        /// \brief Draws the menu the folder in view opens on right click, which is where creating happens.
        void DrawFolderMenu();

        /// \brief Draws the prompt that names a folder before it is created.
        void DrawFolderPrompt();

        /// \brief Enumerates what the naming prompt is about to create.
        enum class Creation : UInt8
        {
            None,       ///< Nothing is being created.
            Folder,     ///< A folder is waiting on its name.
            Material,   ///< A material is waiting on its name.
        };

        /// \brief Reloads the asset an entry stands for, so an edit made outside the editor is picked up.
        ///
        /// \param Item The entry to reload.
        void Reload(ConstRef<Entry> Item);

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

        Str             mFolder;
        Sequence<Entry> mEntries;
        Str             mSelection;
        Creation        mCreating;
        Str             mCreation;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Importer        mImporter;
        Glaze           mGlaze;
    };
}