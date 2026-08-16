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
#include <Zyphryon.Graphic/Image.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Holds the art every terrain in the world draws with.
    class Tileset final : public Engine::Locator<Content::Service>
    {
    public:

        /// \brief The maximum number of motif a tileset can hold (must match the limit in repository).
        static constexpr UInt32 kLimit    = 2'048;

        /// \brief The filename of the tileset data file.
        static constexpr Symbol kFilename = "Resources://Data/Tileset.bin";

        /// \brief Names the run of slices a bake wrote a motif's frames into.
        struct Placement final
        {
            /// The unique identifier of the motif the run belongs to.
            UInt16 Motif  = 0;

            /// The first slice of the run.
            UInt16 Base   = 0;

            /// The number of slices the run occupies, one per animation frame.
            UInt16 Frames = 0;

            /// The baked array the run lives in.
            UInt16 Atlas  = 0;
        };

        /// \brief Everything a draw needs to put a terrain on the ground.
        struct Glyph final
        {
            /// The array holding the motif's frames of each texture, zero where the sheet has none.
            Array<Graphic::Object, Motif::kMaxSources> Textures;

            /// The first slice of the run holding the motif's frames.
            UInt16                                     Start  = 0;

            /// The slice the motif shows right now, which is \ref Start plus the active keyframe.
            UInt16                                     Slice  = 0;

            /// The number of slices the run occupies, one per animation frame.
            UInt16                                     Count  = 0;

            /// Copy of \c Motif::GetPeriod — the tiles the art covers before it repeats.
            IntVector2                                 Period = IntVector2::One();

            /// Copy of \c Motif::GetTint — color multiplier applied at render time.
            IntColor8                                  Tint   = IntColor8::Transparent();

            /// \brief Gets the array holding the motif's frames of a texture.
            ///
            /// \param Slot The texture to read.
            /// \return The array, or zero when the sheet was fired without that texture.
            ZY_INLINE Graphic::Object GetTexture(Motif::Source Slot) const
            {
                return Textures[Enum::Cast(Slot)];
            }
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

        /// \brief Advances every motif's animation, and binds any bake that has finished arriving.
        ///
        /// \param Time The absolute time of the scene, in seconds.
        void Tick(Real64 Time);

        /// \brief Replaces the runs the motifs draw from, as a fresh bake laid them out.
        ///
        /// \param Placements The run every motif the bake covered was written into.
        void Rebind(AnyRef<Sequence<Placement>> Placements);

        /// \brief Copies the properties of one motif onto another.
        ///
        /// \param Source The unique identifier of the motif to copy from.
        /// \param Target The unique identifier of the motif to copy onto.
        void Clone(UInt16 Source, UInt16 Target);

        /// \brief Gets a reference to the motif of a terrain, authoring one when it has none.
        ///
        /// \param ID The unique identifier of the terrain to retrieve the motif of.
        /// \return A reference to the motif associated with the terrain.
        ZY_INLINE Ref<Motif> GetMotif(UInt16 ID)
        {
            if (!mRegistry.IsAllocated(ID))
            {
                mRegistry.Acquire(ID, ID, IntVector2::One(), IntColor8::White());
            }
            return mRegistry[ID];
        }

        /// \brief Gets the glyph a terrain draws with.
        ///
        /// \param ID The unique identifier of the terrain to retrieve the glyph of.
        /// \return A reference to the glyph associated with the terrain.
        ZY_INLINE ConstRef<Glyph> GetGlyph(UInt16 ID) const
        {
            return mGlyphs[ID];
        }

        /// \brief Gets the atlas a run names, adding it to the tileset when it is not held yet.
        ///
        /// \param Url The url of the atlas.
        /// \return The index the runs name the atlas by.
        UInt16 GetOrInsertAtlas(AnyRef<Content::Uri> Url);

        /// \brief Gets the array one of the atlases was fired into for a texture.
        ///
        /// \param Index The atlas to read.
        /// \param Slot  The texture whose array is wanted.
        /// \return The array, or nothing when no run names it or the sheet was fired without that texture.
        ZY_INLINE Retainer<Graphic::Image> GetAtlas(UInt16 Index, Motif::Source Slot) const
        {
            return (Index < mAtlases.GetSize()) ? mAtlases[Index][Enum::Cast(Slot)] : nullptr;
        }

        /// \brief Iterates over every motif a project authored.
        ///
        /// \param Callback The callback function to apply to each motif.
        template<typename Function>
        ZY_INLINE void ForEachMotif(AnyRef<Function> Callback) const
        {
            mRegistry.ForEach(Callback);
        }

        /// \brief Iterates over the run of slices every placed motif draws its frames from.
        ///
        /// \param Callback The callback function to apply to each placement.
        template<typename Function>
        ZY_INLINE void ForEachPlacement(AnyRef<Function> Callback) const
        {
            for (ConstRef<Placement> Placement : mPlacements)
            {
                Callback(Placement);
            }
        }

    public:

        /// \brief Gets the url the array of a texture is baked to, derived from the albedo array's own.
        ///
        /// \param Albedo The url of the albedo array the sheet was fired into.
        /// \param Slot   The texture whose array is wanted.
        /// \return The url of that texture's array, which is the albedo one for \ref Motif::Source::Albedo.
        ZY_INLINE Str GetAtlasUrl(Text Albedo, Motif::Source Slot)
        {
            if (Slot == Motif::Source::Albedo)
            {
                return Albedo;
            }

            // The suffix sits before the extension, so the array keeps the type the loader picks it up by.
            const SInt32 Dot = StrFindLast(Albedo, '.');

            return Dot == -1
                ? Str::Print<"{0}_n">(Albedo)
                : Str::Print<"{0}_n{1}">(Albedo.Slice(0, Dot), Albedo.Slice(Dot));
        }

    private:

        /// \brief The arrays one sheet was fired into, one per texture and all sharing its slice layout.
        using Atlases = Array<Retainer<Graphic::Image>, Motif::kMaxSources>;

        /// \brief Requests the baked array each placement names, dropping whatever a previous bake left.
        void Request();

        /// \brief Hands every placed motif the run of slices the bake wrote its frames into.
        void Update();

        /// \brief Loads the tileset database from file.
        void LoadDatabase(Filesystem::Result Result, Blob Data);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Pool<Motif, kLimit, 0> mRegistry;
        Array<Glyph, kLimit>   mGlyphs;
        Sequence<Atlases>      mAtlases;
        Sequence<Placement>    mPlacements;
        Bool                   mDirty;
    };
}