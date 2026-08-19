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

#include "AssetBaker.hpp"
#include "Tileon.Editor/Context.hpp"
#include "Tileon.Editor/Toolkit/Explorer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Bakes a file sitting anywhere on disk into a folder of the project.
    class Importer final
    {
    public:

        /// \brief Constructs the importer with the specified context.
        ///
        /// \param Context The context associated with this importer.
        explicit Importer(Ref<Context> Context);

        /// \brief Opens the explorer that chooses the source of an import.
        void Browse();

        /// \brief Draws the prompt an import in flight waits on, at the scope of the window it belongs to.
        ///
        /// \param Folder The folder the bake writes into.
        /// \return `true` if the bake wrote into the folder, `false` otherwise.
        Bool DrawPrompt(Text Folder);

        /// \brief Draws the explorer that chooses the source, which is a window of its own.
        void DrawExplorer();

    private:

        /// \brief Gets the baker that accepts a source.
        ///
        /// \param Source The file the user chose.
        /// \return The baker for the file, or `nullptr` when nothing bakes that kind.
        Ptr<AssetBaker> Reach(Text Source) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Context>                 mContext;
        Toolkit::Explorer            mExplorer;
        Sequence<Unique<AssetBaker>> mBakers;
        Ptr<AssetBaker>              mPending;
        Str                          mImport;
    };
}