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

#include <Zyphryon.Math/Geometry/Rect.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents an animation consisting of multiple frames, each with its own coordinates and duration.
    class Animation final
    {
    public:

        /// \brief The maximum number of frames that an animation can contain.
        static constexpr UInt8 kMaxFrames = 12;

    public:

        /// \brief Checks if the animation sequence has reached its maximum capacity of frames.
        ///
        /// \return `true` if the animation sequence is full, `false` otherwise.
        ZY_INLINE Bool IsFull() const
        {
            return mFrames.IsFull();
        }

        /// \brief Checks if the animation sequence is empty, meaning it contains no frames.
        ///
        /// \return `true` if the animation sequence is empty, `false` otherwise.
        ZY_INLINE Bool IsEmpty() const
        {
            return  mFrames.IsEmpty();
        }

        /// \brief Gets the total duration of the sequence by summing the durations of all frames.
        ///
        /// \return The total duration of the sequence in seconds.
        ZY_INLINE Real32 GetDuration() const
        {
            Real32 Total = 0.0f;

            for (ConstRef<Frame> Frame : mFrames)
            {
                Total += Frame.Duration;
            }
            return Total;
        }

        /// \brief Gets the total number of frames currently stored in the sequence.
        ///
        /// \return The number of frames in the sequence.
        ZY_INLINE UInt8 GetCount() const
        {
            return mFrames.GetSize();
        }

        /// \brief Retrieves the keyframe index corresponding to a given time in the animation sequence.
        ///
        /// \param Time The time in seconds for which to retrieve the keyframe index.
        /// \return The index of the keyframe that corresponds to the specified time in the animation sequence.
        ZY_INLINE UInt8 GetKeyframe(Real64 Time) const
        {
            Real64 Elapsed = 0.0;

            for (UInt32 Keyframe = 0; Keyframe < mFrames.GetSize(); ++Keyframe)
            {
                Elapsed += mFrames[Keyframe].Duration;

                if (Time < Elapsed)
                {
                    return Keyframe;
                }
            }
            return mFrames.GetSize() - 1;
        }

        /// \brief Inserts a new frame into the sequence.
        ///
        /// \param Value    The value of the frame.
        /// \param Duration The duration of the frame in seconds.
        ZY_INLINE void Insert(Rect Value, Real32 Duration)
        {
            mFrames.Append(Value, Duration);
        }

        /// \brief Removes a frame from the sequence at the specified keyframe index.
        ///
        /// \param Keyframe The index of the keyframe to remove from the sequence.
        ZY_INLINE void Remove(UInt8 Keyframe)
        {
            mFrames.Remove(Keyframe);
        }

        /// \brief Clears all frames from the sequence, resetting it to an empty state.
        ZY_INLINE void Clear()
        {
            mFrames.Clear();
        }

        /// \brief Updates the coordinates of an existing frame in the sequence at the specified keyframe index.
        ///
        /// \param Keyframe The index of the keyframe to update.
        /// \param Data     The new coordinates to assign to the frame at the specified keyframe index.
        ZY_INLINE void SetFrameData(UInt8 Keyframe, Rect Data)
        {
            mFrames[Keyframe].Data = Data;
        }

        /// \brief Gets the coordinates of a frame from the sequence at the specified keyframe index.
        ///
        /// \param Keyframe The index of the keyframe to retrieve the coordinates from.
        /// \return The coordinates of the frame at the specified keyframe index.
        ZY_INLINE Rect GetFrameData(UInt8 Keyframe) const
        {
            return mFrames[Keyframe].Data;
        }

        /// \brief Updates the duration of an existing frame in the sequence at the specified keyframe index.
        ///
        /// \param Keyframe The index of the keyframe to update.
        /// \param Duration The new duration to assign to the frame at the specified keyframe index
        ZY_INLINE void SetFrameDuration(UInt8 Keyframe, Real32 Duration)
        {
            mFrames[Keyframe].Duration = Duration;
        }

        /// \brief Gets the duration of a frame from the sequence at the specified keyframe index.
        ///
        /// \param Keyframe The index of the keyframe to retrieve the duration from.
        /// \return The duration of the frame at the specified keyframe index.
        ZY_INLINE Real32 GetFrameDuration(UInt8 Keyframe) const
        {
            return mFrames[Keyframe].Duration;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZY_INLINE void Serialize(Serializer Archive)
        {
            Archive.Serialize(mFrames);
        }

    private:

        /// \brief Represents a single frame in the sequence, containing a value and its duration.
        struct Frame final
        {
            /// The coordinates of the frame, representing the area of the sprite sheet to use for this frame.
            Rect   Data;

            /// The duration of the frame in seconds.
            Real32 Duration;
        };

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Sequence<Frame, kMaxFrames> mFrames;
    };
}
