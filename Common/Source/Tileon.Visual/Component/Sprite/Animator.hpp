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

#include "Animation.hpp"
#include <Zyphryon.Math/Easing.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Visual
{
    /// \brief Represents an animator that manages the playback of animations.
    class Animator final
    {
    public:

        /// \brief Represents the playback status of the animation.
        enum class Status : UInt8
        {
            Idle,      ///< Animation is not playing.
            Forward,   ///< Animation is playing forward.
            Backward,  ///< Animation is playing backward.
            Repeat,    ///< Animation is repeating from the beginning.
            Mirror,    ///< Animation alternates between forward and backward playback.
        };

    public:

        /// \brief Initializes the animator with default values.
        ZYPHRYON_INLINE Animator()
            : mTimestamp { 0.0 },
              mStatus    { Status::Idle },
              mEasing    { Easing::Linear },
              mKeyframe  { 0 }
        {
        }

        /// \brief Constructs an animator with the specified timestamp, status, and easing function.
        ///
        /// \param Timestamp The initial timestamp of the animator.
        /// \param Status    The playback status of the animator.
        /// \param Easing    The easing function to use for keyframe interpolation (default is linear).
        ZYPHRYON_INLINE Animator(Real64 Timestamp, Status Status, Easing Easing = Easing::Linear)
            : mTimestamp { Timestamp },
              mStatus    { Status },
              mEasing    { Easing },
              mKeyframe  { 0 }
        {
        }

        /// \brief Advances the animator based on the elapsed time and the properties of the provided sequence.
        ///
        /// \param Time     The total elapsed time since the animator started.
        /// \param Sequence The sequence to advance the animator with, providing frame count and duration.
        void Advance(Real64 Time, ConstRef<Animation> Sequence);

        /// \brief Sets the currtent timestamp of the animator.
        ///
        /// \param Timestamp The new timestamp value.
        ZYPHRYON_INLINE void SetTimestamp(Real64 Timestamp)
        {
            mTimestamp = Timestamp;
        }

        /// \brief Gets the current timestamp of the animator.
        ///
        /// \return The current timestamp.
        ZYPHRYON_INLINE Real64 GetTimestamp() const
        {
            return mTimestamp;
        }

        /// \brief Sets the playback status of the animator.
        ///
        /// \param Status The new playback status.
        ZYPHRYON_INLINE void SetStatus(Status Status)
        {
            mStatus = Status;
        }

        /// \brief Gets the current playback status of the animator.
        ///
        /// \return The current playback status.
        ZYPHRYON_INLINE Status GetStatus() const
        {
            return mStatus;
        }

        /// \brief Sets the easing function for the animator.
        ///
        /// \param Easing The new easing function.
        ZYPHRYON_INLINE void SetEasing(Easing Easing)
        {
            mEasing = Easing;
        }

        /// \brief Gets the easing function of the animator.
        ///
        /// \return The current easing function.
        ZYPHRYON_INLINE Easing GetEasing() const
        {
            return mEasing;
        }

        /// \brief Sets the current keyframe of the animator.
        ///
        /// \param Keyframe The new keyframe index.
        ZYPHRYON_INLINE void SetKeyframe(UInt8 Keyframe)
        {
            mKeyframe = Keyframe;
        }

        /// \brief Gets the current keyframe of the animator.
        ///
        /// \return The current keyframe index.
        ZYPHRYON_INLINE UInt8 GetKeyframe() const
        {
            return mKeyframe;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
        {
            Archive.SerializeReal64(mTimestamp);
            Archive.SerializeEnum(mStatus);
            Archive.SerializeEnum(mEasing);
            Archive.SerializeUInt(mKeyframe);
        }

    private:

        /// \brief Applies the easing function to the given delta time based on the duration of the animation.
        ///
        /// \param Delta    The time delta to apply easing to.
        /// \param Duration The total duration of the animation.
        /// \return The eased time delta, clamped to the range [0, Duration].
        ZYPHRYON_INLINE Real64 ApplyEasing(Real64 Delta, Real64 Duration) const
        {
            return Clamp(Math::Ease(mEasing, Delta / Duration), 0.0, 1.0) * Duration;
        }

    public:

        /// \brief Samples the keyframe of the animation sequence at a specific time, using the provided timestamp and easing function.
        ///
        /// \param Sequence  The animation sequence to sample from, providing frame count and duration.
        /// \param Time      The total elapsed time since the animator started.
        /// \param Timestamp The initial timestamp to use for the animator (default is 0.0).
        /// \param Easing    The easing function to use for keyframe interpolation (default is linear).
        /// \return The keyframe index corresponding to the sampled time in the animation sequence.
        template<Status Status = Status::Forward>
        ZYPHRYON_INLINE static UInt8 Sample(ConstRef<Animation> Sequence, Real64 Time, Real64 Timestamp = 0.0, Easing Easing = Easing::Linear)
        {
            Animator Animator(Timestamp, Status, Easing);
            Animator.Advance(Time, Sequence);
            return Animator.GetKeyframe();
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Real64 mTimestamp;
        Status mStatus;
        Easing mEasing;
        UInt8  mKeyframe;
    };
}
