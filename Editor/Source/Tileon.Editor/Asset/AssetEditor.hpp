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

#include <Zyphryon.Content/Uri.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Authors one kind of asset, as a window of its own.
    class AssetEditor
    {
    public:

        /// \brief Destroys the editor.
        virtual ~AssetEditor() = default;

        /// \brief Gets the suffix of the kind this editor authors.
        ///
        /// \return The extension, including its dot.
        virtual Text GetExtension() const = 0;

        /// \brief Opens an asset, reading what it already holds.
        ///
        /// \param Path The path on disk to read the asset from.
        /// \param Key  The url the asset is loaded under.
        virtual void Open(Text Path, AnyRef<Content::Uri> Key) = 0;

        /// \brief Opens a new asset at the given path.
        ///
        /// \param Path The path on disk the asset will be written to.
        /// \param Key  The url the asset is loaded under.
        /// \return `true` when a file was written there and then, so the folder in view has to be reread.
        virtual Bool Create(Text Path, AnyRef<Content::Uri> Key) = 0;

        /// \brief Draws the editor, which does nothing while nothing is open.
        ///
        /// \return `true` when the asset was written this frame, so the folder in view has to be reread.
        virtual Bool Draw() = 0;
    };
}