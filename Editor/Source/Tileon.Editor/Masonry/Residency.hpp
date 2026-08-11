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
#include <Zyphryon.Graphic/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Masonry
{
    /// \brief Keeps the motifs authored since the last bake drawable, by copying their frames into live arrays.
    class Residency final : public Engine::Locator<Graphic::Service>
    {
    public:

        /// \brief The number of slices a live array is created with, and grows by when it fills.
        static constexpr UInt16 kSlices = 32;

    public:

        /// \brief Constructs a residency over the tileset whose glyphs it binds.
        ///
        /// \param Host    The service host to associate with the residency.
        /// \param Tileset The tileset whose motifs are copied into arrays.
        /// \param Gallery The cache holding the sheets the frames are copied out of.
        explicit Residency(Ref<Engine::Subsystem::Host> Host, Ref<Tileset> Tileset, Ref<Gallery> Gallery);

        /// \brief Destroys the arrays the editor built, which no bake ever wrote to disk.
        ~Residency();

        /// \brief Copies the frames of every motif that has none into a live array.
        void Tick();

    private:

        /// \brief Gathers the motif images sharing one texture signature into a single array texture.
        struct Atlas final
        {
            /// A contiguous run of slices, handed out per motif so its frames stay adjacent.
            struct Run final
            {
                /// The first slice of the run.
                UInt16 Base  = 0;

                /// The number of slices the run occupies, one per animation frame.
                UInt16 Count = 0;
            };

            /// The array texture holding the frames.
            Graphic::Object        Texture  = 0;

            /// The pixel format every frame in the array shares.
            Graphic::TextureFormat Format   = Graphic::TextureFormat::Unspecified;

            /// The width of every frame in the array, in pixels.
            UInt16                 Width    = 0;

            /// The height of every frame in the array, in pixels.
            UInt16                 Height   = 0;

            /// The number of mip levels every frame in the array carries.
            UInt8                  Levels   = 0;

            /// The number of slices the array was created with.
            UInt16                 Capacity = 0;

            /// The number of slices handed out, which is also the next unused slice.
            UInt16                 Count    = 0;

            /// The runs returned by motifs that let go of their frames, reused when a length matches.
            Sequence<Run>          Recycled;
        };

        /// \brief Copies a motif's frames into a run of array slices, taking one if the glyph has none.
        ///
        /// \param Motif The motif whose frames are copied.
        /// \param Glyph The glyph to promote, which does nothing until its image has finished uploading.
        void Promote(ConstRef<Motif> Motif, Ref<Tileset::Glyph> Glyph);

        /// \brief Returns the run a motif took to the array it came from.
        ///
        /// \param ID The unique identifier of the motif whose run is reclaimed.
        void Demote(UInt16 ID);

        /// \brief Finds the array holding images of the given signature, creating one when none matches.
        ///
        /// \param Format The pixel format the array must hold.
        /// \param Width  The width of one frame, in pixels.
        /// \param Height The height of one frame, in pixels.
        /// \param Levels The number of mip levels each frame carries.
        /// \param Count  The number of contiguous slices the caller is about to take.
        /// \return The array accepting the frames, grown beforehand when it had too few slices left.
        UInt16 Acquire(Graphic::TextureFormat Format, UInt16 Width, UInt16 Height, UInt8 Levels, UInt16 Count);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Tileset>                               mTileset;
        Ref<Gallery>                               mGallery;
        Sequence<Atlas>                            mAtlases;
        Array<Tileset::Placement, Tileset::kLimit> mLive;
    };
}