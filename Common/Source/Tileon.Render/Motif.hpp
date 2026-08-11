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

#include "Component/Animator.hpp"
#include <Zyphryon.Content/Uri.hpp>
#include <Zyphryon.Math/Color.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents the art a terrain draws with.
    class Motif final
    {
    public:

        /// \brief Constructs an empty motif.
        ZY_INLINE Motif()
            : mID     { 0 },
              mPeriod { IntVector2::One() },
              mTint   { IntColor8::White() },
              mEasing { Easing::Linear }
        {
        }

        /// \brief Constructs a motif with the specified unique identifier.
        ///
        /// \param ID     The unique identifier for the motif.
        /// \param Period The extent of the motif's art, in whole tiles.
        /// \param Tint   The tint color to apply to the motif when rendering.
        ZY_INLINE Motif(UInt16 ID, IntVector2 Period, IntColor8 Tint)
            : mID     { ID },
              mPeriod { Period },
              mTint   { Tint },
              mEasing { Easing::Linear }
        {
        }

        /// \brief Gets the unique identifier for the motif.
        ///
        /// \return The unique identifier for the motif.
        ZY_INLINE UInt16 GetID() const
        {
            return mID;
        }

        /// \brief Sets the URI of the material resource the motif's frames are cut from.
        ///
        /// \param Material The URI of the material resource.
        ZY_INLINE void SetMaterial(AnyRef<Content::Uri> Material)
        {
            mMaterial = Move(Material);
        }

        /// \brief Gets the URI of the material resource the motif's frames are cut from.
        ///
        /// \return The URI of the material resource.
        ZY_INLINE ConstRef<Content::Uri> GetMaterial() const
        {
            return mMaterial;
        }

        /// \brief Sets the extent of the motif's art, in whole tiles.
        ///
        /// \param Period The period to set, clamped so it can always divide a coordinate.
        ZY_INLINE void SetPeriod(IntVector2 Period)
        {
            mPeriod = IntVector2(Max<SInt32>(Period.GetX(), 1), Max<SInt32>(Period.GetY(), 1));
        }

        /// \brief Gets the extent of the motif's art, in whole tiles.
        ///
        /// \return The number of tiles the art covers before it repeats, never less than one.
        ZY_INLINE IntVector2 GetPeriod() const
        {
            return mPeriod;
        }

        /// \brief Sets the tint color of the motif.
        ///
        /// \param Tint The new tint color to set for the motif.
        ZY_INLINE void SetTint(IntColor8 Tint)
        {
            mTint = Tint;
        }

        /// \brief Gets the tint color of the motif.
        ///
        /// \return The tint color of the motif.
        ZY_INLINE IntColor8 GetTint() const
        {
            return mTint;
        }

        /// \brief Sets the easing function for the motif's animation.
        ///
        /// \param Easing The new easing function to set for the motif's animation.
        ZY_INLINE void SetEasing(Easing Easing)
        {
            mEasing = Easing;
        }

        /// \brief Gets the easing function for the motif's animation.
        ///
        /// \return The easing function for the motif's animation.
        ZY_INLINE Easing GetEasing() const
        {
            return mEasing;
        }

        /// \brief Sets the animation data for the motif.
        ///
        /// \param Animation The new animation data to set for the motif.
        ZY_INLINE void SetAnimation(AnyRef<Animation> Animation)
        {
            mAnimation = Move(Animation);
        }

        /// \brief Gets the animation data for the motif.
        ///
        /// \return The animation data for the motif.
        ZY_INLINE ConstRef<Animation> GetAnimation() const
        {
            return mAnimation;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZY_INLINE void Serialize(Serializer Archive)
        {
            Archive.Serialize(mID);
            Archive.Serialize(mMaterial);
            Archive.Serialize(mPeriod);
            Archive.Serialize(mTint);
            Archive.Serialize(mEasing);
            Archive.Serialize(mAnimation);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UInt16       mID;
        Content::Uri mMaterial;
        IntVector2   mPeriod;
        IntColor8    mTint;
        Easing       mEasing;
        Animation    mAnimation;
    };
}