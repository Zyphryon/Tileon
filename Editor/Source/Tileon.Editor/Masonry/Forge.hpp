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

#include "Bakery.hpp"
#include "Residency.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Masonry
{
    /// \brief Owns everything the editor needs to author a tileset, and is the only handle onto it.
    class Forge final
    {
    public:

        /// \brief Constructs a forge over the tileset it authors.
        ///
        /// \param Host    The service host to associate with the forge.
        /// \param Tileset The tileset the motifs are authored into.
        explicit Forge(Ref<Engine::Subsystem::Host> Host, Ref<Tileset> Tileset);

        /// \brief Copies the frames of every motif authored since the last bake into a live array.
        void Tick();

        /// \brief Requests the sheet of every motif, reporting whether they have all settled.
        ///
        /// \return `true` once every sheet has settled, `false` while any is still loading.
        Bool Prepare();

        /// \brief Writes every motif's frames into the arrays a project ships, and binds the tileset to them.
        ///
        /// \param Folder The project folder the arrays and the sheets they cut from are rooted at.
        /// \return `true` if every array was assembled, `false` otherwise.
        Bool Bake(Text Folder);

        /// \brief Gets a reference to the sheets the motifs were authored on.
        ///
        /// \return A reference to the sheets the forge holds.
        ZY_INLINE Ref<Gallery> GetGallery()
        {
            return mGallery;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Gallery   mGallery;
        Residency mResidency;
        Bakery    mBakery;
    };
}