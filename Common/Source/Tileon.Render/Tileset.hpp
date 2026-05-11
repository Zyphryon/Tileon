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

#include "Motif.hpp"
#include <Zyphryon.Content/Service.hpp>
#include <Zyphryon.Graphic/Material.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a tileset that manages the rendering data for different terrains in the world.
    class Tileset final : public Locator<Content::Service>
    {
    public:

        /// \brief The maximum number of motif a tileset can hold (must match the limit in repository).
        static constexpr UInt32    kLimit    = 1'024;

        /// \brief The filename of the tileset data file.
        static constexpr ConstStr8 kFilename = "Resources://Data/Tileset.bin";

        /// \brief Defines a glyph structure that represents the visual representation of a motif.
        struct Glyph final
        {
            /// \brief The cached material used for rendering the motif.
            Tracker<Graphic::Material> Material;

            /// \brief The active frame of the motif's animation, used for animated tiles.
            Rect                       Crop = Rect::Zero();

            /// \brief Copy of \c Motif::Span — tile extent in columns and rows.
            IntVector2                 Span = IntVector2();

            /// \brief Copy of \c Motif::Tint — color multiplier applied at render time.
            IntColor8                  Tint = IntColor8::Transparent();
        };

    public:

        /// \brief Constructs a tileset with the specified service host.
        ///
        /// \param Host The service host to associate with the tileset.
        explicit Tileset(Ref<Service::Host> Host);

        /// \brief Loads the tile data from the tileset file.
        void Load();

        /// \brief Saves the tile data to the tileset file.
        void Save();

        /// \brief Updates the animations of the tileset based on the elapsed time.
        ///
        /// \param Time The elapsed time since the last update, in seconds.
        void Tick(Real64 Time);

        /// \brief Preloads all motifs in the tileset, building their glyph caches.
        void Preload();

        /// \brief Refreshes the glyph cache for a single motif.
        ///
        /// \param Motif The motif whose glyph cache should be rebuilt.
        void Refresh(ConstRef<Motif> Motif);

        /// \brief Gets a reference to the motif associated with the specified terrain.
        ///
        /// \param ID The unique identifier of the terrain to retrieve.
        /// \return A reference to the motif associated with the specified terrain.
        ZYPHRYON_INLINE Ref<Motif> GetMotif(UInt16 ID)
        {
            if (!mRegistry.IsAllocated(ID))
            {
                mRegistry.Acquire(ID, ID);
            }
            return mRegistry[ID];
        }

        ///\brief Gets a reference to the glyph associated with the specified motif.
        ///
        /// \param ID The unique identifier of the motif to retrieve the glyph for.
        /// \return A reference to the glyph associated with the specified motif.
        ZYPHRYON_INLINE ConstRef<Glyph> GetGlyph(UInt16 ID) const
        {
            LOG_ASSERT(ID < mGlyphs.size(), "Exceeded motif limit of tileset: ", ID);
            return mGlyphs[ID];
        }

    private:

        /// \brief Loads the tileset database from file.
        ///
        /// \return `true` if the database was loaded successfully, `false` otherwise.
        Bool LoadDatabase();

        /// \brief Saves the tileset database to file.
        void SaveDatabase();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Pool<Motif, kLimit>  mRegistry;
        Array<Glyph, kLimit> mGlyphs;
    };
}