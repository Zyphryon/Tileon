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
    class Tileset final : public Engine::Locator<Content::Service>
    {
    public:

        /// \brief The maximum number of motif a tileset can hold (must match the limit in repository).
        static constexpr UInt32 kLimit    = 1'024;

        /// \brief The filename of the tileset data file.
        static constexpr Text   kFilename = "Resources://Data/Tileset.bin";

        /// \brief Defines a glyph structure that represents the visual representation of a motif.
        struct Glyph final
        {
            /// \brief The cached material used for rendering the motif.
            Retainer<Graphic::Material> Material;

            /// \brief The active frame of the motif's animation, used for animated tiles.
            Rect                        Crop = Rect::Zero();

            /// \brief Copy of \c Motif::Span — tile extent in columns and rows.
            IntVector2                  Span = IntVector2();

            /// \brief Copy of \c Motif::Tint — color multiplier applied at render time.
            IntColor8                   Tint = IntColor8::Transparent();
        };

    public:

        /// \brief Constructs a tileset with the specified service host.
        ///
        /// \param Host The service host to associate with the tileset.
        explicit Tileset(Ref<Engine::Subsystem::Host> Host);

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

        /// \brief Clones the properties of one motif to another.
        ///
        /// \param Source The unique identifier of the source motif.
        /// \param Target The unique identifier of the target motif.
        void Clone(UInt16 Source, UInt16 Target);

        /// \brief Gets a reference to the motif associated with the specified terrain.
        ///
        /// \param ID The unique identifier of the terrain to retrieve.
        /// \return A reference to the motif associated with the specified terrain.
        ZY_INLINE Ref<Motif> GetMotif(UInt16 ID)
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
        ZY_INLINE ConstRef<Glyph> GetGlyph(UInt16 ID) const
        {
            return mGlyphs[ID];
        }

        /// \brief Gets the revision of the glyph cache.
        ///
        /// \return A counter bumped on every glyph refresh, letting derived caches detect staleness.
        ZY_INLINE UInt32 GetGeneration() const
        {
            return mGeneration;
        }

    private:

        /// \brief Loads the tileset database from file.
        void LoadDatabase(Filesystem::Result Result, Blob Data);

        /// \brief Saves the tileset database to file.
        void SaveDatabase();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Pool<Motif, kLimit>  mRegistry;
        Array<Glyph, kLimit> mGlyphs;
        UInt32               mGeneration;
    };
}