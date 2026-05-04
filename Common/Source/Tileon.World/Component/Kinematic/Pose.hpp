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

#include <Zyphryon.Math/Matrix3x2.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents the pose of an entity in the world, encapsulating its translation, scale, and rotation.
    class Pose final
    {
    public:

        /// \brief Default constructor initializing the pose with no translation, unit scale, and no rotation.
        ZYPHRYON_INLINE Pose()
            : mScale { Vector2::One() }
        {
        }

        /// \brief Initializes the pose with the specified translation, scale, and rotation.
        ///
        /// \param Translation The translation vector to set for the pose (2D).
        /// \param Scale       The scale vector to set for the pose (2D).
        /// \param Rotation    The rotation to set for the pose, represented as an angle in radians.
        ZYPHRYON_INLINE Pose(Vector2 Translation, Vector2 Scale, Angle Rotation)
        {
            SetTranslation(Translation);
            SetScale(Scale);
            SetRotation(Rotation);
        }

        /// \brief Initializes the pose with the specified translation and scale, using no rotation.
        ///
        /// \param Translation The translation vector to set for the pose (2D).
        /// \param Scale       The scale vector to set for the pose (2D).
        ZYPHRYON_INLINE Pose(Vector2 Translation, Vector2 Scale)
        {
            SetTranslation(Translation);
            SetScale(Scale);
        }

        /// \brief Initializes the pose with the specified translation, using unit scale and no rotation.
        ///
        /// \param Translation The translation vector to set for the pose (2D).
        ZYPHRYON_INLINE Pose(Vector2 Translation)
            : Pose()
        {
            SetTranslation(Translation);
        }

        /// \brief Computes the transformation matrix for the pose, optionally using a pivot point.
        ///
        /// \param Pivot The pivot point to use for the transformation, which serves as the center of rotation and scaling.
        /// \return A transformation matrix that combines translation, scale, and rotation.
        ZYPHRYON_INLINE Matrix3x2 Compute(Vector2 Pivot = Vector2()) const
        {
            return Matrix3x2::FromTransform(Pivot, mTranslation, mScale, mRotation);
        }

        /// \brief Sets the translation vector for the pose.
        ///
        /// \param Translation The translation vector to set for the pose (2D).
        /// \return A reference to this pose, modified by the new translation.
        ZYPHRYON_INLINE Ref<Pose> SetTranslation(Vector2 Translation)
        {
            mTranslation = Translation;
            return (* this);
        }

        /// \brief Gets the current translation of the pose.
        ///
        /// \return The translation vector of the pose (2D).
        ZYPHRYON_INLINE Vector2 GetTranslation() const
        {
            return mTranslation;
        }

        /// \brief Sets the scale vector for the pose.
        ///
        /// \param Scale The scale vector to set for the pose (2D).
        /// \return A reference to this pose, modified by the new scale.
        ZYPHRYON_INLINE Ref<Pose> SetScale(Vector2 Scale)
        {
            mScale = Scale;
            return (* this);
        }

        /// \brief Gets the current scale of the pose.
        ///
        /// \return The scale vector of the pose (2D).
        ZYPHRYON_INLINE Vector2 GetScale() const
        {
            return mScale;
        }

        /// \brief Sets the rotation for the pose.
        ///
        /// \param Rotation The rotation to set for the pose, represented as an angle in radians.
        /// \return A reference to this pose, modified by the new rotation.
        ZYPHRYON_INLINE Ref<Pose> SetRotation(Angle Rotation)
        {
            mRotation = Rotation;
            return (* this);
        }

        /// \brief Gets the current rotation of the pose.
        ///
        /// \return The rotation of the pose, represented as an angle in radians.
        ZYPHRYON_INLINE Angle GetRotation() const
        {
            return mRotation;
        }

        /// \brief Translates the pose by the given 2D vector.
        ///
        /// \param Translation The translation vector to apply to the pose (2D).
        /// \return A reference to this pose, modified by the translation.
        ZYPHRYON_INLINE Ref<Pose> Translate(Vector2 Translation)
        {
            mTranslation += Translation;
            return (* this);
        }

        /// \brief Scales the pose by the given scalar factor.
        ///
        /// \param Scalar The scalar value to apply as a scale factor to the pose.
        /// \return A reference to this pose, modified by the new scale.
        ZYPHRYON_INLINE Ref<Pose> Scale(Real32 Scalar)
        {
            LOG_ASSERT(Scalar != 0.0f, "Scale factor must not be zero");

            mScale *= Scalar;
            return (* this);
        }

        /// \brief Scales the pose by the given 2D vector.
        ///
        /// \param Vector The scale vector to apply to the pose (2D).
        /// \return A reference to this pose, modified by the new scale.
        ZYPHRYON_INLINE Ref<Pose> Scale(Vector2 Vector)
        {
            LOG_ASSERT(Vector.GetX() != 0.0f, "Scale X must not be zero");
            LOG_ASSERT(Vector.GetY() != 0.0f, "Scale Y must not be zero");

            mScale *= Vector;
            return (* this);
        }

        /// \brief Rotates the pose by the given angle.
        ///
        /// \param Rotation The angle to apply as a rotation to the pose, represented in radians.
        /// \return A reference to this pose, modified by the new rotation.
        ZYPHRYON_INLINE Ref<Pose> Rotate(Angle Rotation)
        {
            mRotation += Rotation;
            return (* this);
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        /// 
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
        {
            Archive.SerializeObject(mTranslation);
            Archive.SerializeObject(mScale);
            Archive.SerializeObject(mRotation);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Vector2 mTranslation;
        Vector2 mScale;
        Angle   mRotation;
    };
}