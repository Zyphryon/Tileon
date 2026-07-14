// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

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

    public:

        /// \brief Sets the properties of a specific layer in the tile.
        ///
        /// \param Type   The type of layer to set.
        /// \param Handle The unique identifier for the terrain type of the layer.
        /// \param Weight The weight of the layer, used for multi-span terrains.
        ZY_INLINE void SetLayer(Layer Type, UInt16 Handle, UInt16 Weight)
        {
            mLayers[Enum::Cast(Type)] = Unit(Handle, Weight);
        }

        /// \brief Gets the unique identifier for the terrain type of a specific layer in the tile.
        ///
        /// \param Type The type of layer to retrieve the handle for.
        /// \return The unique identifier for the terrain type of the specified layer.
        ZY_INLINE UInt16 GetHandle(Layer Type) const
        {
            return mLayers[Enum::Cast(Type)].Handle;
        }

        /// \brief Gets the weight of a specific layer in the tile, used for multi-span terrains.
        ///
        /// \param Type The type of layer to retrieve the weight for.
        /// \return The weight of the specified layer.
        ZY_INLINE UInt16 GetWeight(Layer Type) const
        {
            return mLayers[Enum::Cast(Type)].Weight;
        }

    private:

        /// \brief Represents a single layer unit within the tile.
        struct Unit final
        {
            /// \brief The unique identifier for the terrain type of this layer unit.
            UInt16 Handle;

            /// \brief The weight of the layer unit, used for multi-span terrains.
            UInt16 Weight;
        };

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Array<Unit, Enum::Count<Layer>()> mLayers;
    };
}