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

#include <Zyphryon.Math/Vector2.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a single tile in the tilemap, consisting of multiple layers.
    class Tile final
    {
    public:

        /// \brief Enumerates the different layers that can be assigned to a tile.
        enum class Layer : UInt8
        {
            Base,       ///< The base layer of the tile, representing the primary terrain type.
            Detail,     ///< The detail layer of the tile, representing additional terrain features or variations.
        };

        /// \brief Define where the art of a layer is anchored, in whole tiles along each axis.
        using Offset = AnyVector2<UInt8>;

        /// \brief Represents what a single layer of a tile draws.
        struct Unit final
        {
            /// The unique identifier for the terrain type of the layer.
            UInt16 Handle;

            /// Where the terrain's art is anchored.
            Offset Offset;
        };

    public:

        /// \brief Sets the terrain a specific layer of the tile shows.
        ///
        /// \param Type   The type of layer to set.
        /// \param Handle The unique identifier for the terrain type of the layer.
        /// \param Offset The alignment offset, zero to follow the lattice the world defines.
        ZY_INLINE void SetLayer(Layer Type, UInt16 Handle, Offset Offset)
        {
            mLayers[Enum::Cast(Type)] = Unit(Handle, Offset);
        }

        /// \brief Gets the unique identifier for the terrain type of a specific layer in the tile.
        ///
        /// \param Type The type of layer to retrieve the handle for.
        /// \return The unique identifier for the terrain type of the specified layer.
        ZY_INLINE UInt16 GetHandle(Layer Type) const
        {
            return mLayers[Enum::Cast(Type)].Handle;
        }

        /// \brief Gets where the art of a specific layer of the tile is anchored.
        ///
        /// \param Type The type of layer to retrieve the offset of.
        /// \return The alignment offset.
        ZY_INLINE Offset GetOffset(Layer Type) const
        {
            return mLayers[Enum::Cast(Type)].Offset;
        }

    public:

        /// \brief Reduces an anchor into the period of a motif.
        ///
        /// \param Anchor The anchor, in whole tiles.
        /// \param Period The motif's period, in whole tiles.
        /// \return The anchor along each axis, always inside the period.
        ZY_INLINE static IntVector2 Align(IntVector2 Anchor, IntVector2 Period)
        {
            const SInt32 X = Anchor.GetX() % Period.GetX();
            const SInt32 Y = Anchor.GetY() % Period.GetY();
            return IntVector2(X < 0 ? X + Period.GetX() : X, Y < 0 ? Y + Period.GetY() : Y);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Array<Unit, Enum::Count<Layer>()> mLayers;
    };
}