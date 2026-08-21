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

#include "Sprite.hpp"
#include <Zyphryon.Graphic/Material.hpp>
#include <Zyphryon.Math/Geometry/Rect.hpp>

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
        ZY_INLINE Appearance() = default;

        /// \brief Constructs an appearance with the specified material and source rectangle.
        ///
        /// \param Material   The material to use for rendering the sprite.
        /// \param Source     The source rectangle for the sprite.
        /// \param Resolution The pixel dimensions of the material's albedo, which the source rect is a fraction of.
        /// \param Facing     The orientation of the sprite.
        /// \param Plane      The plane the art is laid against.
        ZY_INLINE Appearance(
            ConstRetainer<Graphic::Material> Material,
            Rect                             Source,
            Vector2                          Resolution,
            Sprite::Facing                   Facing,
            Sprite::Plane                    Plane)
            : mMaterial   { Material },
              mSource     { Source },
              mResolution { Resolution },
              mFacing     { Facing },
              mPlane      { Plane }
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

        /// \brief Sets the pixel dimensions of the albedo the source crops out of.
        ///
        /// \param Resolution The pixel dimensions of the material's albedo, which the source rect is a fraction of.
        ZY_INLINE void SetResolution(Vector2 Resolution)
        {
            mResolution = Resolution;
        }

        /// \brief Gets the pixel dimensions of the albedo the source crops out of.
        ///
        /// \return The dimensions of the material's albedo, or zero while it is still loading.
        ZY_INLINE Vector2 GetResolution() const
        {
            return mResolution;
        }

        /// \brief Sets the orientation of the sprite.
        ///
        /// \param Facing The orientation to lay the sprite down with.
        ZY_INLINE void SetFacing(Sprite::Facing Facing)
        {
            mFacing = Facing;
        }

        /// \brief Gets the orientation of the sprite.
        ///
        /// \return The orientation the sprite is laid down with.
        ZY_INLINE Sprite::Facing GetFacing() const
        {
            return mFacing;
        }

        /// \brief Sets the plane the art is laid against.
        ///
        /// \param Plane The plane the art is laid against.
        ZY_INLINE void SetPlane(Sprite::Plane Plane)
        {
            mPlane = Plane;
        }

        /// \brief Gets the plane the art is laid against.
        ///
        /// \return The plane the art is laid against.
        ZY_INLINE Sprite::Plane GetPlane() const
        {
            return mPlane;
        }

        /// \brief Gets the bits a technique tests for how the art is laid down.
        ///
        /// \note The same bits are named to the shaders by the `FACING_` defines a geometry technique declares.
        ///
        /// \return The mirroring of the art, the plane it spans, and whether it shares that plane with the ground.
        ZY_INLINE UInt32 GetFacingID() const
        {
            const UInt32 Coplanar = (mPlane == Sprite::Plane::Ground ? 1u << 4u : 0u);
            return Enum::Cast(mFacing) | (Enum::Cast(mPlane) << 2u) | Coplanar;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Retainer<Graphic::Material> mMaterial;
        Rect                        mSource;
        Vector2                     mResolution;
        Sprite::Facing              mFacing;
        Sprite::Plane               mPlane;
    };
}