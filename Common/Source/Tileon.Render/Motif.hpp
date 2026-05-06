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

#include "Component/Sprite/Animator.hpp"
#include <Zyphryon.Content/Uri.hpp>
#include <Zyphryon.Math/Color.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a motif, which is the visual element for a terrain tile.
    class Motif final
    {
    public:

        /// \brief Default constructor.
        ZYPHRYON_INLINE Motif() = default;

        /// \brief Constructs a motif with the specified unique identifier, span, and tint color.
        ///
        /// \param ID   The unique identifier for the motif.
        /// \param Span The span of the motif, representing how many tiles it covers (default is 1x1).
        /// \param Tint The tint color to apply to the motif when rendering (default is white).
        ZYPHRYON_INLINE Motif(UInt16 ID, IntVector2 Span = IntVector2::One(), IntColor8 Tint = IntColor8::White())
            : mID     { ID   },
              mSpan   { Span },
              mTint   { Tint },
              mEasing { Easing::Linear }
        {
        }

        /// \brief Gets the unique identifier for the motif.
        ///
        /// \return The unique identifier for the motif.
        ZYPHRYON_INLINE UInt16 GetID() const
        {
            return mID;
        }

        /// \brief Sets the URI of the material resource used to render this motif.
        ///
        /// \param Material The URI of the material resource.
        ZYPHRYON_INLINE void SetMaterial(AnyRef<Content::Uri> Material)
        {
            mMaterial = Move(Material);
        }

        /// \brief Gets the URI of the material resource used to render this motif.
        ///
        /// \return The URI of the material resource.
        ZYPHRYON_INLINE ConstRef<Content::Uri> GetMaterial() const
        {
            return mMaterial;
        }

        /// \brief Sets the span of the motif.
        ///
        /// \param Span The new span to set for the motif.
        ZYPHRYON_INLINE void SetSpan(IntVector2 Span)
        {
            mSpan = Span;
        }

        /// \brief Gets the span of the motif.
        ///
        /// \return The span of the motif.
        ZYPHRYON_INLINE IntVector2 GetSpan() const
        {
            return mSpan;
        }

        /// \brief Sets the tint color of the motif.
        ///
        /// \param Tint The new tint color to set for the motif.
        ZYPHRYON_INLINE void SetTint(IntColor8 Tint)
        {
            mTint = Tint;
        }

        /// \brief Gets the tint color of the motif.
        ///
        /// \return The tint color of the motif.
        ZYPHRYON_INLINE IntColor8 GetTint() const
        {
            return mTint;
        }

        /// \brief Sets the easing function for the motif's animation.
        ///
        /// \param Easing The new easing function to set for the motif's animation.
        ZYPHRYON_INLINE void SetEasing(Easing Easing)
        {
            mEasing = Easing;
        }

        /// \brief Gets the easing function for the motif's animation.
        ///
        /// \return The easing function for the motif's animation.
        ZYPHRYON_INLINE Easing GetEasing() const
        {
            return mEasing;
        }

        /// \brief Sets the animation data for the motif.
        ///
        /// \param Animation The new animation data to set for the motif.
        ZYPHRYON_INLINE void SetAnimation(AnyRef<Animation> Animation)
        {
            mAnimation = Move(Animation);
        }

        /// \brief Gets the animation data for the motif.
        ///
        /// \return The animation data for the motif.
        ZYPHRYON_INLINE ConstRef<Animation> GetAnimation() const
        {
            return mAnimation;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
        {
            Archive.SerializeUInt16(mID);
            Archive.SerializeObject(mMaterial);
            Archive.SerializeObject(mSpan);
            Archive.SerializeObject(mTint);
            Archive.SerializeEnum(mEasing);
            Archive.SerializeObject(mAnimation);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UInt16       mID;
        Content::Uri mMaterial;
        IntVector2   mSpan;
        IntColor8    mTint;
        Easing       mEasing;
        Animation    mAnimation;
    };
}