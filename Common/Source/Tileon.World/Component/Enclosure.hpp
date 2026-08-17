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
    /// \brief Represents the computed world-space axis-aligned volume that encloses an entity.
    class Enclosure final
    {
    public:

        /// \brief The cell of an enclosure that is not filed under one.
        static constexpr IntVector2 kUnlinked = IntVector2(INT32_MIN, INT32_MIN);

    public:

        /// \brief Constructs an empty enclosure that is not yet linked to a spatial cell.
        ZY_INLINE Enclosure()
            : mCell { kUnlinked }
        {
        }

        /// \brief Constructs an enclosure from a world-space volume.
        ///
        /// \param Volume The absolute world-space volume, in world units.
        ZY_INLINE Enclosure(ConstRef<IntBox> Volume)
            : mVolume { Volume },
              mCell   { kUnlinked }
        {
        }

        /// \brief Checks whether the entity is filed under a spatial cell.
        ///
        /// \return `true` if the entity belongs to a cell, `false` otherwise.
        ZY_INLINE Bool IsLinked() const
        {
            return mCell != kUnlinked;
        }

        /// \brief Sets the world-space volume that encloses the entity.
        ///
        /// \param Volume The absolute world-space volume, in world units.
        ZY_INLINE void SetVolume(ConstRef<IntBox> Volume)
        {
            mVolume = Volume;
        }

        /// \brief Gets the world-space volume that encloses the entity.
        ///
        /// \return The absolute world-space volume, in world units.
        ZY_INLINE ConstRef<IntBox> GetVolume() const
        {
            return mVolume;
        }

        /// \brief Sets the spatial cell the entity is filed under.
        ///
        /// \param Cell The cell the entity now belongs to, or \ref kUnlinked for none.
        ZY_INLINE void SetCell(IntVector2 Cell)
        {
            mCell = Cell;
        }

        /// \brief Gets the spatial cell the entity is filed under.
        ///
        /// \return The cell, or \ref kUnlinked if the entity is not filed under one.
        ZY_INLINE IntVector2 GetCell() const
        {
            return mCell;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        IntBox     mVolume;
        IntVector2 mCell;
    };
}