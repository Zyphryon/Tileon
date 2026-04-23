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

        /// \brief Gets the total duration of the sequence by summing the durations of all frames.
        ///
        /// \return The total duration of the sequence in seconds.
        ZYPHRYON_INLINE Real32 GetDuration() const
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
        ZYPHRYON_INLINE UInt8 GetCount() const
        {
            return mFrames.size();
        }

        /// \brief Retrieves the keyframe index corresponding to a given time in the animation sequence.
        ///
        /// \param Time The time in seconds for which to retrieve the keyframe index.
        /// \return The index of the keyframe that corresponds to the specified time in the animation sequence.
        ZYPHRYON_INLINE UInt8 GetKeyframe(Real64 Time) const
        {
            Real64 Elapsed = 0.0;

            for (UInt32 Keyframe = 0; Keyframe < mFrames.size(); ++Keyframe)
            {
                Elapsed += mFrames[Keyframe].Duration;

                if (Time < Elapsed)
                {
                    return Keyframe;
                }
            }
            return mFrames.size() - 1;
        }

        /// \brief Inserts a new frame into the sequence.
        ///
        /// \param Value    The value of the frame.
        /// \param Duration The duration of the frame in seconds.
        ZYPHRYON_INLINE void Insert(Rect Value, Real32 Duration)
        {
            mFrames.emplace_back(Value, Duration);
        }

        /// \brief Removes a frame from the sequence at the specified keyframe index.
        ///
        /// \param Keyframe The index of the keyframe to remove from the sequence.
        ZYPHRYON_INLINE void Remove(UInt8 Keyframe)
        {
            mFrames.erase(mFrames.begin() + Keyframe);
        }

        /// \brief Clears all frames from the sequence, resetting it to an empty state.
        ZYPHRYON_INLINE void Clear()
        {
            mFrames.clear();
        }

        /// \brief Updates the coordinates of an existing frame in the sequence at the specified keyframe index.
        ///
        /// \param Keyframe The index of the keyframe to update.
        /// \param Data     The new coordinates to assign to the frame at the specified keyframe index.
        ZYPHRYON_INLINE void SetFrameData(UInt8 Keyframe, Rect Data)
        {
            mFrames[Keyframe].Data = Data;
        }

        /// \brief Gets the coordinates of a frame from the sequence at the specified keyframe index.
        ///
        /// \param Keyframe The index of the keyframe to retrieve the coordinates from.
        /// \return The coordinates of the frame at the specified keyframe index.
        ZYPHRYON_INLINE Rect GetFrameData(UInt8 Keyframe) const
        {
            return mFrames[Keyframe].Data;
        }

        /// \brief Updates the duration of an existing frame in the sequence at the specified keyframe index.
        ///
        /// \param Keyframe The index of the keyframe to update.
        /// \param Duration The new duration to assign to the frame at the specified keyframe index
        ZYPHRYON_INLINE void SetFrameDuration(UInt8 Keyframe, Real32 Duration)
        {
            mFrames[Keyframe].Duration = Duration;
        }

        /// \brief Gets the duration of a frame from the sequence at the specified keyframe index.
        ///
        /// \param Keyframe The index of the keyframe to retrieve the duration from.
        /// \return The duration of the frame at the specified keyframe index.
        ZYPHRYON_INLINE Real32 GetFrameDuration(UInt8 Keyframe) const
        {
            return mFrames[Keyframe].Duration;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
        {
            Archive.SerializeVector(mFrames);
        }

    private:

        /// \brief Represents a single frame in the sequence, containing a value and its duration.
        struct Frame final
        {
            /// \brief The coordinates of the frame, representing the area of the sprite sheet to use for this frame.
            Rect   Data;

            /// \brief The duration of the frame in seconds.
            Real32 Duration;

            /// \brief Constructs an empty frame with default values.
            ZYPHRYON_INLINE Frame()
                : Duration { 0.0f }
            {
            }

            /// \brief Constructs a frame with the specified value and duration.
            ///
            /// \param Data     The coordinates of the frame, representing the area of the sprite sheet.
            /// \param Duration The duration of the frame in seconds.
            ZYPHRYON_INLINE Frame(Rect Data, Real32 Duration)
                : Data     { Data },
                  Duration { Duration }
            {
            }

            /// \brief Serializes the state of the object to or from the specified archive.
            ///
            /// \param Archive The archive to serialize the object with.
            template<typename Serializer>
            ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
            {
                Archive.SerializeObject(Data);
                Archive.SerializeReal32(Duration);
            }
        };

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Vector<Frame, kMaxFrames> mFrames;
    };
}
