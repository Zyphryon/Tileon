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
    /// \brief Represents the spatial extent of an entity in the world, defined by an offset and size.
    class Extent final
    {
    public:

        /// \brief Default constructor initializing the extent with zero offset and size.
        ZYPHRYON_INLINE Extent() = default;

        /// \brief Constructs an extent with the specified offset and size.
        ///
        /// \param Offset The offset of the extent, representing the position relative to the entity's origin.
        /// \param Size   The size of the extent, representing the width and height of the entity's spatial area.
        ZYPHRYON_INLINE Extent(Vector2 Offset, Vector2 Size)
            : mOffset { Offset },
              mSize   { Size }
        {
        }

        /// \brief Sets the offset of the extent.
        ///
        /// \param Offset The offset to set for the extent.
        ZYPHRYON_INLINE void SetOffset(Vector2 Offset)
        {
            mOffset = Offset;
        }

        /// \brief Gets the offset of the extent.
        ///
        /// \return The offset of the extent.
        ZYPHRYON_INLINE Vector2 GetOffset() const
        {
            return mOffset;
        }

        /// \brief Sets the size of the extent.
        ///
        /// \param Size The size to set for the extent.
        ZYPHRYON_INLINE void SetSize(Vector2 Size)
        {
            mSize = Size;
        }

        /// \brief Gets the size of the extent.
        ///
        /// \return The size of the extent.
        ZYPHRYON_INLINE Vector2 GetSize() const
        {
            return mSize;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
        {
            Archive.SerializeObject(mOffset);
            Archive.SerializeObject(mSize);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Vector2 mOffset;
        Vector2 mSize;
    };
}
