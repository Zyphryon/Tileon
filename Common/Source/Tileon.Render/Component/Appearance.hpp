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
#include <Zyphryon.Scene/Tag.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents the appearance of a sprite that contains runtime data for a sprite entity.
    class Appearance final
    {
    public:

        /// \brief Default constructor.
        ZY_INLINE Appearance()
            : mSource { },
              mSheet  { Vector2::Zero() }
        {
        }

        /// \brief Constructs an appearance with the specified material and source rectangle.
        ///
        /// \param Material The material to use for rendering the sprite.
        /// \param Source   The source rectangle for the sprite.
        /// \param Sheet    The dimensions of the material's albedo.
        ZY_INLINE Appearance(ConstRetainer<Graphic::Material> Material, Rect Source, Vector2 Sheet)
            : mMaterial { Material },
              mSource   { Source },
              mSheet    { Sheet }
        {
        }

        /// \brief Sets the material for rendering the sprite.
        ///
        /// \param Material The material to set for rendering the sprite.
        ZY_INLINE void SetMaterial(ConstRetainer<Graphic::Material> Material)
        {
            mMaterial = Material;
        }

        /// \brief Gets the material used for rendering the sprite.
        ///
        /// \return The material used for rendering the sprite.
        ZY_INLINE ConstRetainer<Graphic::Material> GetMaterial() const
        {
            return mMaterial;
        }

        /// \brief Sets the source rectangle for the sprite.
        ///
        /// \param Source The source rectangle to set for the sprite.
        ZY_INLINE void SetSource(Rect Source)
        {
            mSource = Source;
        }

        /// \brief Gets the source rectangle for the sprite.
        ///
        /// \return The source rectangle for the sprite.
        ZY_INLINE Rect GetSource() const
        {
            return mSource;
        }

        /// \brief Sets the size of the sheet the source crops out of, in pixels.
        ///
        /// \param Sheet The dimensions of the material's albedo.
        ZY_INLINE void SetSheet(Vector2 Sheet)
        {
            mSheet = Sheet;
        }

        /// \brief Gets the size of the sheet the source crops out of, in pixels.
        ///
        /// \return The dimensions of the material's albedo, or zero while it is still loading.
        ZY_INLINE Vector2 GetSheet() const
        {
            return mSheet;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Retainer<Graphic::Material> mMaterial;
        Rect                        mSource;
        Vector2                     mSheet;
    };
}