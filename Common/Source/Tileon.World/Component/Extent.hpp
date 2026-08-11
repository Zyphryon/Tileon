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

#include <Zyphryon.Math/Geometry/Box.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents the spatial extent of an entity in the world, defined by an offset and size.
    class Extent final
    {
    public:

        /// \brief Constructs an extent with no offset and no size.
        ZY_INLINE Extent() = default;

        /// \brief Constructs an extent from a local-space offset and size.
        ///
        /// \param Offset The corner of the volume, relative to the entity's own origin.
        /// \param Size   The dimensions of the volume along each axis.
        ZY_INLINE Extent(Vector3 Offset, Vector3 Size)
            : mOffset { Offset },
              mSize   { Size }
        {
        }

        /// \brief Sets the corner of the volume in local space.
        ///
        /// \param Offset The corner of the volume, relative to the entity's own origin.
        ZY_INLINE void SetOffset(Vector3 Offset)
        {
            mOffset = Offset;
        }

        /// \brief Gets the corner of the volume in local space.
        ///
        /// \return The corner of the volume, relative to the entity's own origin.
        ZY_INLINE Vector3 GetOffset() const
        {
            return mOffset;
        }

        /// \brief Sets the dimensions of the volume.
        ///
        /// \param Size The dimensions of the volume along each axis.
        ZY_INLINE void SetSize(Vector3 Size)
        {
            mSize = Size;
        }

        /// \brief Gets the dimensions of the volume.
        ///
        /// \return The dimensions of the volume along each axis.
        ZY_INLINE Vector3 GetSize() const
        {
            return mSize;
        }

        /// \brief Composes the offset and size into a local-space volume.
        ///
        /// \return The volume the entity occupies in its own local space.
        ZY_INLINE Box GetVolume() const
        {
            return Box(mOffset, mOffset + mSize);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Vector3 mOffset;
        Vector3 mSize;
    };
}