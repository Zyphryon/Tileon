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

#include "Tileon.Render/Tileset.hpp"
#include <Zyphryon.Graphic/Material.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Masonry
{
    /// \brief Holds the sheets the motifs of a tileset were authored on, which a game never loads.
    class Gallery final : public Engine::Locator<Content::Service>
    {
    public:

        /// \brief A motif's sheet, and the extent of the frames it cuts out of it.
        struct Cut final
        {
            /// The albedo the frames are cut out of, empty while it is still loading or the motif names none.
            Retainer<Graphic::Image> Image;

            /// The width of one frame, in pixels, which every frame of the motif shares.
            UInt16                   Width;

            /// The height of one frame, in pixels, which every frame of the motif shares.
            UInt16                   Height;

            /// The number of frames the motif's animation names.
            UInt16                   Frames;

            /// \brief Constructs an empty cut, which names no sheet and no frames.
            ZY_INLINE Cut()
                : Width  { 0 },
                  Height { 0 },
                  Frames { 0 }
            {
            }

            /// \brief Checks whether the sheet arrived and names frames of a drawable extent.
            ///
            /// \return `true` if the cut can be copied from, `false` otherwise.
            ZY_INLINE Bool IsValid() const
            {
                return Image && Frames > 0 && Width > 0 && Height > 0;
            }
        };

    public:

        /// \brief Constructs a cache holding no sheet.
        ///
        /// \param Host The service host to associate with the cache.
        explicit Gallery(Ref<Engine::Subsystem::Host> Host);

        /// \brief Resolves a motif's sheet and measures the frames it cuts out of it.
        ///
        /// \param Motif The motif to measure.
        /// \return The cut behind the motif, invalid while its sheet has yet to arrive.
        Cut Measure(ConstRef<Motif> Motif);

        /// \brief Requests a motif's sheet and reports whether it has settled, either arrived or failed.
        ///
        /// \param Motif The motif whose sheet is awaited.
        /// \return `true` once the sheet has settled, `false` while it is still loading.
        Bool Settle(ConstRef<Motif> Motif);

        /// \brief Gets the sheet a motif's frames were authored on, which the panels that edit it draw from.
        ///
        /// \param ID The unique identifier of the motif whose sheet is retrieved.
        /// \return The sheet behind the motif, or an empty handle while it has none.
        ZY_INLINE ConstRetainer<Graphic::Material> GetMaterial(UInt16 ID) const
        {
            return mSheets[ID];
        }

    private:

        /// \brief Gets the sheet a motif names, loading it the first time it is asked for.
        ///
        /// \param Motif The motif whose sheet is requested.
        /// \return The sheet behind the motif, empty when it names no art.
        Ref<Retainer<Graphic::Material>> Request(ConstRef<Motif> Motif);

        /// \brief Gets the image a motif cuts its frames out of, loading its sheet the first time it is asked for.
        ///
        /// \param Motif The motif whose sheet is resolved.
        /// \return The albedo behind the motif, or an empty handle while it is still loading or has none.
        Retainer<Graphic::Image> Resolve(ConstRef<Motif> Motif);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Array<Retainer<Graphic::Material>, Tileset::kLimit> mSheets;
    };
}