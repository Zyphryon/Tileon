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

#include <Zyphryon.Graphic/Material.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a palette of materials, allowing for efficient material management within a region.
    class Palette final
    {
    public:

        /// \brief The maximum number of materials that can be stored in the palette.
        static constexpr UInt32 kCapacity = 16;

    public:

        /// \brief Initializes an empty palette.
        Palette() = default;

        /// \brief Sets a material at the specified index in the palette.
        ///
        /// \param Index    The index at which to set the material.
        /// \param Material The material to set at the specified index.
        ZYPHRYON_INLINE void SetMaterial(UInt32 Index, ConstTracker<Graphic::Material> Material)
        {
            LOG_ASSERT(Index < kCapacity, "Palette index {} is out of bounds (Capacity: {})", Index, kCapacity);

            mPalette[Index] = Material;
        }

        /// \brief Retrieves the material at the specified index in the palette.
        ///
        /// \param Index The index of the material to retrieve.
        /// \return The material at the specified index.
        ZYPHRYON_INLINE ConstTracker<Graphic::Material> GetMaterial(UInt32 Index) const
        {
            LOG_ASSERT(Index < kCapacity, "Palette index {} is out of bounds (Capacity: {})", Index, kCapacity);

            return mPalette[Index];
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Array<Tracker<Graphic::Material>, kCapacity> mPalette;
    };
}
