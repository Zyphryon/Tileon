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

#include "Gallery.hpp"
#include <Manifest.hpp>
#include <Zyphryon.Job/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Masonry
{
    /// \brief Writes the array textures a project ships, cut from the sheets its motifs were authored on.
    class Bakery final : public Engine::Locator<Job::Service>
    {
    public:

        /// \brief The path, under the project folder, of the array a group of frames is assembled into.
        static constexpr Symbol kAtlas = "{0}/Material/Tileset.{1}.tex";

    public:

        /// \brief Constructs a bakery over the tileset whose runs it lays out.
        ///
        /// \param Host    The service host to associate with the bakery.
        /// \param Tileset The tileset whose motifs are written into arrays.
        /// \param Gallery  The cache holding the sheets the frames are cut from.
        explicit Bakery(Ref<Engine::Subsystem::Host> Host, Ref<Tileset> Tileset, Ref<Gallery> Gallery);

        /// \brief Requests the sheet of every motif, reporting whether they have all settled.
        ///
        /// \return `true` once every sheet has settled, `false` while any is still loading.
        Bool Prepare();

        /// \brief Writes every motif's frames into the arrays a project ships, and binds the tileset to them.
        ///
        /// \param Folder The project folder the arrays and the sheets they cut from are rooted at.
        /// \return `true` if every array was assembled, `false` otherwise.
        Bool Bake(Text Folder);

    private:

        /// Shorthand for the manifest entry naming one frame's crop on the sheet it is cut from.
        using Entry = ::Pipeline::Baker::Image::Manifest::Entry;

        /// \brief One array's worth of frames, all of them cut to the same size, and the entries naming them.
        struct Group final
        {
            /// The width of every frame in the group, in pixels.
            UInt16          Width  = 0;

            /// The height of every frame in the group, in pixels.
            UInt16          Height = 0;

            /// The number of slices handed out, which is also the next unused slice.
            UInt16          Count  = 0;

            /// The entries naming every frame the group assembles, in slice order.
            Sequence<Entry> Entries;
        };

        /// \brief Sorts every motif's frames into the group sharing their extent, naming the run each one takes.
        ///
        /// \param Folder     The project folder the sheets the frames are cut from are rooted at.
        /// \param Groups     The groups the frames are gathered into, one per distinct frame extent.
        /// \param Placements The run every motif the walk covered was laid out into.
        /// \return `true` if every motif was gathered, `false` otherwise.
        Bool Gather(Text Folder, Ref<Sequence<Group>> Groups, Ref<Sequence<Tileset::Placement>> Placements);

        /// \brief Assembles each group into an array texture and writes it under the project folder.
        ///
        /// \param Folder The project folder the arrays are written to.
        /// \param Groups The groups to assemble, one per array.
        /// \return `true` if every array was written, `false` otherwise.
        Bool Assemble(Text Folder, ConstRef<Sequence<Group>> Groups);

        /// \brief Deletes the arrays left beside the ones a bake just wrote, which it no longer names.
        ///
        /// \param Folder The project folder the arrays are written to.
        /// \param Count  The number of arrays the bake wrote, which is also the first one to delete.
        void Discard(Text Folder, UInt32 Count);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Tileset> mTileset;
        Ref<Gallery> mGallery;
    };
}