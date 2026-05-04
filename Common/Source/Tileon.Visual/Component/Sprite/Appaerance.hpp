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
#include <Zyphryon.Math/Geometry/Rect.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Visual
{
    /// \brief Represents the appearance of a sprite that contains runtime data for a sprite entity.
    class Appaerance final
    {
    public:

        /// \brief Default constructor.
        ZYPHRYON_INLINE Appaerance() = default;

        /// \brief Constructs an appearance with the specified material and source rectangle.
        ///
        /// \param Material The material to use for rendering the sprite.
        /// \param Source   The source rectangle for the sprite, defining the portion of the texture to use.
        ZYPHRYON_INLINE Appaerance(ConstTracker<Graphic::Material> Material, Rect Source)
            : mMaterial { Material },
              mSource   { Source }
        {
        }

        /// \brief Sets the material for rendering the sprite.
        ///
        /// \param Material The material to set for rendering the sprite.
        ZYPHRYON_INLINE void SetMaterial(ConstTracker<Graphic::Material> Material)
        {
            mMaterial = Material;
        }

        /// \brief Gets the material used for rendering the sprite.
        ///
        /// \return The material used for rendering the sprite.
        ZYPHRYON_INLINE ConstTracker<Graphic::Material> GetMaterial() const
        {
            return mMaterial;
        }

        /// \brief Sets the source rectangle for the sprite.
        ///
        /// \param Source The source rectangle to set for the sprite, defining the portion of the texture to use.
        ZYPHRYON_INLINE void SetSource(Rect Source)
        {
            mSource = Source;
        }

        /// \brief Gets the source rectangle for the sprite.
        ///
        /// \return The source rectangle for the sprite, defining the portion of the texture to use.
        ZYPHRYON_INLINE Rect GetSource() const
        {
            return mSource;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Tracker<Graphic::Material> mMaterial;
        Rect                       mSource;
    };
}
