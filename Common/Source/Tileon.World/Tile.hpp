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

        /// \brief Defines the different layers that a tile can have.
        enum class Layer : UInt8
        {
            Base,       ///< The base layer of the tile, typically representing the ground or floor.
            Detail,     ///< The overlay layer, used for additional details like paths or markings.
        };

        /// \brief The size of the tile in pixels (width and height).
        static constexpr SInt8 kExtent = 32;

    public:

        /// \brief Constructs an empty tile with no layers set.
        ZYPHRYON_INLINE Tile()
            : mLayers { 0 }
        {
        }

        /// \brief Sets the properties of a specific layer in the tile.
        ///
        /// \param Type    The type of layer to set.
        /// \param Handle  The terrain identifier associated with this layer.
        /// \param Weight  The rendering weight of this layer.
        /// \param Palette The palette index to use for this layer.
        ZYPHRYON_INLINE void SetLayer(Layer Type, UInt16 Handle, UInt8 Weight, UInt8 Palette)
        {
            mLayers[Enum::Cast(Type)] = Unit(Handle, Weight, Palette);
        }

        /// \brief Gets the terrain identifier of a specific layer in the tile.
        ///
        /// \param Type The type of layer to query.
        /// \return The terrain identifier.
        ZYPHRYON_INLINE UInt16 GetHandle(Layer Type) const
        {
            return mLayers[Enum::Cast(Type)].Handle;
        }

        /// \brief Gets the rendering weight of a specific layer in the tile.
        ///
        /// \param Type The type of layer to query.
        /// \return The weight value.
        ZYPHRYON_INLINE UInt8 GetWeight(Layer Type) const
        {
            return mLayers[Enum::Cast(Type)].Weight;
        }

        /// \brief Retrieves the palette index of a specific layer in the tile.
        ///
        /// \param Type The type of layer to query.
        /// \return The palette index.
        ZYPHRYON_INLINE UInt8 GetPalette(Layer Type) const
        {
            return mLayers[Enum::Cast(Type)].Palette;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
        {
            Archive.SerializeArray(mLayers);
        }

    private:

        /// \brief Represents a single layer unit within the tile.
        struct Unit final
        {
            /// \brief Terrain handle identifier for this layer.
            UInt16 Handle;

            /// \brief Rendering weight for this layer (0-255).
            UInt8  Weight;

            /// \brief Rendering palette index for this layer (0-255).
            UInt8  Palette;

            /// \brief Serializes the state of the object to or from the specified archive.
            ///
            /// \param Archive The archive to serialize the object with.
            template<typename Serializer>
            ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
            {
                Archive.SerializeUInt(Handle);
                Archive.SerializeUInt8(Weight);
                Archive.SerializeUInt8(Palette);
            }
        };

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Array<Unit, Enum::Count<Layer>()> mLayers;
    };
}