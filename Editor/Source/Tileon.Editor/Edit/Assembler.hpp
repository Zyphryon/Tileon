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

        /// \brief Constructs an assembler writing the terrains of the specified splatset.
        ///
        /// \param Context  The context the project, graphic service and scheduler come from.
        /// \param Splatset The splatset whose arrays are written.
        Assembler(Ref<Context> Context, Ref<Splatset> Splatset);

        /// \brief Gets one of the arrays the splatset's material binds.
        ///
        /// \param Usage The array to get.
        /// \return The array, or `nullptr` while the material has yet to load.
        Retainer<Graphic::Image> GetArray(Texture Usage) const;

        /// \brief Adds one terrain, as a slice appended to each of the arrays.
        ///
        /// \param Albedo The art the terrain is coloured by.
        /// \param Normal The art the terrain takes its relief from, which may be empty.
        /// \param Name   The name to author the terrain under, which may be empty.
        /// \return `true` when the terrain was added, `false` when the bake was refused or failed.
        Bool Append(Text Albedo, Text Normal, Text Name);

        /// \brief Bakes a normal map into the slice a terrain already occupies.
        ///
        /// \param Slice  The slice to write, which the terrain already draws its colour from.
        /// \param Source The path of the art to take the relief from.
        /// \return `true` when the relief was written, `false` when the art could not be read or matched.
        Bool AppendRelief(UInt16 Slice, Text Source);

        /// \brief Writes the arrays back out with every slice added since they were last assembled.
        void Commit();

    private:

        /// \brief The number of slices a bake leaves room for beyond the terrains it holds.
        static constexpr UInt16 kHeadroom = 8;

        /// \brief Holds the art of one terrain added since the arrays were last written out.
        struct Pending final
        {
            /// The resolved path of the terrain's base color.
            Str Albedo;

            /// The resolved path of the terrain's surface normals, which may be empty.
            Str Normal;
        };

        /// \brief Bakes one terrain's art straight into the arrays the splatset already holds.
        ///
        /// \param Albedo The resolved path of the terrain's base color.
        /// \param Normal The resolved path of the terrain's surface normals, which may be empty.
        /// \param Name   The name to author the terrain under.
        /// \return `true` when the terrain was written, `false` when its art could not be read or matched.
        Bool AppendInPlace(Text Albedo, Text Normal, Text Name);

        /// \brief Writes flat relief into one slice, for a terrain nobody gave a normal map.
        ///
        /// \param Array The array of surface normals to write into.
        /// \param Slice The slice to make flat.
        void WriteFlatRelief(ConstRetainer<Graphic::Image> Array, UInt16 Slice);

        /// \brief Writes every level of one slice of an array the material holds.
        ///
        /// \param Array  The array to write into.
        /// \param Slice  The slice to write.
        /// \param Pixels The levels of the slice, one after another, in the format the array holds.
        void WriteSlice(ConstRetainer<Graphic::Image> Array, UInt16 Slice, ConstSpan<Byte> Pixels);

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
        Sequence<Pending>  mPending;
        Table<UInt16, Str> mReliefs;
    };
}