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

#include "Tileon.Editor/Context.hpp"
#include "Tileon.Render/Terrain/Splatset.hpp"
#include "Tileon.Render/Types.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Writes the art of every terrain into the arrays the ground samples.
    class Assembler final
    {
    public:

        /// \brief The arts one terrain is authored from.
        ///
        /// \note Height is not an array of its own; it rides in the alpha of the colour.
        enum class Slot : UInt8
        {
            Albedo,     ///< The colour the terrain shows.
            Normal,     ///< The relief it is lit by.
            Height,     ///< The elevation it stands at.
        };

        /// \brief Constructs an assembler writing the terrains of the specified splatset.
        ///
        /// \param Context  The context the project, graphic service and scheduler come from.
        /// \param Splatset The splatset whose arrays are written.
        Assembler(Ref<Context> Context, Ref<Splatset> Splatset);

        /// \brief Adds a terrain holding nothing, reserving the slice each array draws it from.
        ///
        /// \return The slice the new terrain draws, or #Splatset::kInvalid when none could be had.
        UInt16 Create();

        /// \brief Names the art one of a terrain's slots is baked from, and bakes it.
        ///
        /// \param Slice  The slice the terrain draws.
        /// \param Slot   Which of the terrain's arts is being named.
        /// \param Source The path of the art, or empty to take that art away again.
        /// \return `true` when the art was written, `false` when it could not be read.
        Bool SetSource(UInt16 Slice, Slot Slot, Text Source);

        /// \brief Bakes every terrain again, from the art each of them named, at the authored size.
        ///
        /// \return `true` when every array was written back.
        Bool Rebuild();

        /// \brief Writes the arrays back out with every slice added since they were last assembled.
        void Commit();

    private:

        /// \brief The number of slices a bake leaves room for beyond the terrains it holds.
        static constexpr UInt16 kHeadroom = 8;

        /// \brief Writes every level of one slice of an array the material holds.
        ///
        /// \param Array  The array to write into.
        /// \param Slice  The slice to write.
        /// \param Pixels The levels of the slice, one after another, in the format the array holds.
        void WriteSlice(ConstRetainer<Graphic::Image> Array, UInt16 Slice, ConstSpan<Byte> Pixels);

        /// \brief Gets one of the arrays the splatset's material binds, once it has loaded.
        ///
        /// \param Usage The array to get.
        /// \return The array, or `nullptr` while the material has yet to load.
        Retainer<Graphic::Image> GetArray(Texture Usage) const;

        /// \brief Bakes the art one slot of one terrain names into the slice it already holds.
        ///
        /// \param Slice The slice to write.
        /// \param Usage The array to write into.
        /// \return `true` when the slice was written.
        Bool WriteArt(UInt16 Slice, Texture Usage);

        /// \brief Writes the material naming the arrays the ground samples, and what they carry.
        ///
        /// \param Lit    Whether the project carries an array of surface normals to bind.
        /// \param Raised Whether any terrain baked a height into the alpha of its colour.
        void WriteMaterial(Bool Lit, Bool Raised);

        /// \brief Resolves a mounted path into the one on disk the baker reads and writes.
        ///
        /// \param Path The path to resolve, which may already name a plain one.
        /// \return The path on disk.
        Str Resolve(Text Path);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Context>       mContext;
        Ref<Splatset>      mSplatset;
        Bool               mDirty;
    };
}