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

namespace Tileon::Editor
{
    /// \brief Turns a file sitting anywhere on disk into an asset of the project.
    class AssetBaker
    {
    public:

        /// \brief Destroys the baker.
        virtual ~AssetBaker() = default;

        /// \brief Gets the suffixes of the sources this baker accepts.
        ///
        /// \return The extensions, separated by spaces, in the form the explorer filters by.
        virtual Text GetSources() const = 0;

        /// \brief Gets the human-readable name of what the bake produces.
        ///
        /// \return The label.
        virtual Text GetLabel() const = 0;

        /// \brief Draws the settings the bake waits on, inside the prompt the importer opens.
        virtual void DrawSettings() = 0;

        /// \brief Bakes a source into a folder of the project.
        ///
        /// \param Source The file to bake, which is left where it is.
        /// \param Folder The folder the bake writes into.
        /// \return `true` if anything was written, `false` otherwise.
        virtual Bool Bake(Text Source, Text Folder) = 0;
    };
}