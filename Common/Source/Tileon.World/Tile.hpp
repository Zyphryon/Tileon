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

        /// \brief Enumerates the eight ways the art of a layer can be laid onto the ground.
        enum class Orientation : UInt8
        {
            None      = 0,                   ///< The art as it was authored.
            MirrorX   = 1 << 0,              ///< The art mirrored across the x-axis.
            MirrorY   = 1 << 1,              ///< The art mirrored across the y-axis.
            Transpose = 1 << 2,              ///< The art's two axes exchanged.
            Quarter   = MirrorY | Transpose, ///< The art turned a quarter.
            Opposite  = MirrorX | Transpose, ///< The art turned a quarter the other way.
        };

        /// \brief Define where the art of a layer is anchored, in whole tiles along each axis.
        using Offset = AnyVector2<UInt8>;

        /// \brief Represents what a single layer of a tile draws.
        struct Unit final
        {
            /// The unique identifier for the terrain type of the layer.
            UInt16 Handle      : 12;

            /// How the terrain's art is laid down.
            UInt16 Orientation : 4;

            /// Where the terrain's art is anchored.
            Offset Offset;
        };

    public:

        /// \brief Sets the terrain a specific layer of the tile shows.
        ///
        /// \param Type        The type of layer to set.
        /// \param Handle      The unique identifier for the terrain type of the layer.
        /// \param Offset      The alignment offset, zero to follow the lattice the world defines.
        /// \param Orientation The orientation of the layer.
        ZY_INLINE void SetLayer(Layer Type, UInt16 Handle, Offset Offset, Orientation Orientation)
        {
            mLayers[Enum::Cast(Type)] = Unit(Handle, Enum::Cast(Orientation), Offset);
        }

        /// \brief Gets the unique identifier for the terrain type of a specific layer in the tile.
        ///
        /// \param Type The type of layer to retrieve the handle for.
        /// \return The unique identifier for the terrain type of the specified layer.
        ZY_INLINE UInt16 GetHandle(Layer Type) const
        {
            return mLayers[Enum::Cast(Type)].Handle;
        }

        /// \brief Gets how the art of a specific layer of the tile is laid down.
        ///
        /// \param Type The type of layer to retrieve the orientation of.
        /// \return The orientation the layer's art is laid down with.
        ZY_INLINE Orientation GetOrientation(Layer Type) const
        {
            return static_cast<Orientation>(mLayers[Enum::Cast(Type)].Orientation);
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

        /// \brief Checks whether an orientation applies an operation.
        ///
        /// \param Value The orientation to inspect.
        /// \param Flag  The operation to look for.
        /// \return `true` if the orientation applies the operation, `false` otherwise.
        ZY_INLINE static constexpr Bool Has(Orientation Value, Orientation Flag)
        {
            return HasBit(Enum::Cast(Value), Enum::Cast(Flag));
        }

        /// \brief Applies one orientation on top of another.
        ///
        /// \param Base  The orientation applied first.
        /// \param Delta The orientation applied on top of it.
        /// \return The single orientation equivalent to applying both.
        ZY_INLINE static constexpr Orientation Compose(Orientation Base, Orientation Delta)
        {
            const Bool Swapped = Has(Delta, Orientation::Transpose);
            const Bool MirrorX = Has(Base, Swapped ? Orientation::MirrorY : Orientation::MirrorX);
            const Bool MirrorY = Has(Base, Swapped ? Orientation::MirrorX : Orientation::MirrorY);

            return static_cast<Orientation>(
                  (MirrorX != Has(Delta, Orientation::MirrorX)  ? Enum::Cast(Orientation::MirrorX)   : 0)
                | (MirrorY != Has(Delta, Orientation::MirrorY)  ? Enum::Cast(Orientation::MirrorY)   : 0)
                | (Has(Base, Orientation::Transpose) != Swapped ? Enum::Cast(Orientation::Transpose) : 0));
        }

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