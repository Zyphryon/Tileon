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

#include <Zyphryon.Content/Uri.hpp>
#include <Zyphryon.Graphic/Types.hpp>
#include <Zyphryon.Math/Color.hpp>
#include <Zyphryon.Math/Motion/Flipbook.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents the art a terrain draws with.
    class Motif final
    {
    public:

        /// \brief Names the art a motif is fired from, under the names the tile signature declares.
        enum class Source : UInt8
        {
            Albedo, ///< The base color the tile shows.
            Normal, ///< The tangent-space normal map that gives its surface relief, when it has one.
        };

        /// \brief The maximum number of frames a motif's run can hold.
        static constexpr UInt kMaxFrames  = 11;

        /// \brief The number of textures a motif fires at once, one per \ref Source.
        static constexpr UInt kMaxSources = Enum::Count<Source>();

        /// \brief The run of frames a motif plays, one baked slice each and no data of its own.
        using Flipbook = Flipbook<Empty, kMaxFrames>;

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
        /// \brief Sets the art the motif's frames of a source were last fired from.
        ///
        /// \param Slot   The source the art is fired into.
        /// \param Url    The url of the image the frames were cut from.
        ZY_INLINE void SetSource(Source Slot, AnyRef<Content::Uri> Url)
        {
            mSources[Enum::Cast(Slot)] = Move(Url);
        }

        /// \brief Gets the art the motif's frames of a source were last fired from.
        ///
        /// \param Slot The source to look up.
        /// \return The url of the image the frames were cut from, empty when the motif has none.
        ZY_INLINE ConstRef<Content::Uri> GetSource(Source Slot) const
        {
            return mSources[Enum::Cast(Slot)];
        }

        /// \brief Sets the run of frames the motif plays.
        ///
        /// \param Flipbook The run to play, each frame holding for as long as it says.
        ZY_INLINE void SetFlipbook(ConstRef<Flipbook> Flipbook)
        {
            mFlipbook = Flipbook;
        }

        /// \brief Gets the run of frames the motif plays.
        ///
        /// \return The run the motif plays, which holds nothing when the motif never animates.
        ZY_INLINE ConstRef<Flipbook> GetFlipbook() const
        {
            return mFlipbook;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZY_INLINE void Serialize(Serializer Archive)
        {
            Archive.Serialize(mID);
            Archive.Serialize(mPeriod);
            Archive.Serialize(mTint);
            Archive.Serialize(mEasing);
            Archive.Serialize(mSources);
            Archive.Serialize(mFlipbook);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UInt16                           mID;
        Easing                           mEasing;
        IntColor8                        mTint;
        IntVector2                       mPeriod;
        Array<Content::Uri, kMaxSources> mSources;
        Flipbook                         mFlipbook;
    };
}