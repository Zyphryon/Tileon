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

#include <Zyphryon.Math/Matrix4x3.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents the pose of an entity in the world, encapsulating its translation, scale, and rotation.
    class Pose final
    {
    public:

        /// \brief Constructs a pose at the parent's origin with unit scale and no rotation.
        ZY_INLINE Pose()
            : mScale { Vector3::One() }
        {
        }

        /// \brief Constructs a pose from a translation, scale and rotation.
        ///
        /// \param Translation The position, relative to the parent.
        /// \param Scale       The scale along each axis.
        /// \param Rotation    The rotation, normalized on assignment.
        ZY_INLINE Pose(Vector3 Translation, Vector3 Scale, Quaternion Rotation)
        {
            SetTranslation(Translation);
            SetScale(Scale);
            SetRotation(Rotation);
        }

        /// \brief Constructs an unrotated pose from a translation and scale.
        ///
        /// \param Translation The position, relative to the parent.
        /// \param Scale       The scale along each axis.
        ZY_INLINE Pose(Vector3 Translation, Vector3 Scale)
        {
            SetTranslation(Translation);
            SetScale(Scale);
        }

        /// \brief Constructs a pose from a translation, with unit scale and no rotation.
        ///
        /// \param Translation The position, relative to the parent.
        ZY_INLINE Pose(Vector3 Translation)
            : Pose()
        {
            SetTranslation(Translation);
        }

        /// \brief Composes the pose into a matrix, pivoting around the given point.
        ///
        /// \param Pivot The point scale and rotation are applied around, usually the entity's \ref Anchor.
        /// \return The matrix representing this pose relative to the parent.
        ZY_INLINE Matrix4x3 Compute(Vector3 Pivot = Vector3()) const
        {
            return Matrix4x3::FromTransform(Pivot, mTranslation, mScale, mRotation);
        }

        /// \brief Sets the position of the entity.
        ///
        /// \param Translation The position, relative to the parent.
        /// \return This pose, allowing for method chaining.
        ZY_INLINE Ref<Pose> SetTranslation(Vector3 Translation)
        {
            mTranslation = Translation;
            return (* this);
        }

        /// \brief Gets the position of the entity.
        ///
        /// \return The position, relative to the parent.
        ZY_INLINE Vector3 GetTranslation() const
        {
            return mTranslation;
        }

        /// \brief Sets the scale of the entity.
        ///
        /// \param Scale The scale along each axis.
        /// \return This pose, allowing for method chaining.
        ZY_INLINE Ref<Pose> SetScale(Vector3 Scale)
        {
            mScale = Scale;
            return (* this);
        }

        /// \brief Gets the scale of the entity.
        ///
        /// \return The scale along each axis.
        ZY_INLINE Vector3 GetScale() const
        {
            return mScale;
        }

        /// \brief Sets the rotation of the entity.
        ///
        /// \param Rotation The rotation to apply, normalized on assignment.
        /// \return This pose, allowing for method chaining.
        ZY_INLINE Ref<Pose> SetRotation(Quaternion Rotation)
        {
            mRotation = Quaternion::Normalize(Rotation);
            return (* this);
        }

        /// \brief Gets the rotation of the entity.
        ///
        /// \return The rotation, always normalized.
        ZY_INLINE Quaternion GetRotation() const
        {
            return mRotation;
        }

        /// \brief Sets the rotation of the entity from Euler angles.
        ///
        /// \param Angles The rotation about each axis, in radians.
        /// \return This pose, allowing for method chaining.
        ZY_INLINE Ref<Pose> SetEulerAngles(Vector3 Angles)
        {
            mRotation = Quaternion::FromEulerAngles(Angles);
            return (* this);
        }

        /// \brief Gets the rotation of the entity as Euler angles.
        ///
        /// \return The rotation about each axis, in radians.
        ZY_INLINE Vector3 GetEulerAngles() const
        {
            return mRotation.ToEulerAngles();
        }

        /// \brief Moves the entity by an offset.
        ///
        /// \param Translation The offset to add to the current position.
        /// \return This pose, allowing for method chaining.
        ZY_INLINE Ref<Pose> Translate(Vector3 Translation)
        {
            mTranslation += Translation;
            return (* this);
        }

        /// \brief Scales the entity uniformly by a factor.
        ///
        /// \param Scalar The factor to multiply the current scale by. Must not be zero.
        /// \return This pose, allowing for method chaining.
        ZY_INLINE Ref<Pose> Scale(Real32 Scalar)
        {
            ZY_ASSERT(Scalar != 0.0f, "Scale factor must not be zero");

            mScale *= Scalar;
            return (* this);
        }

        /// \brief Scales the entity per axis.
        ///
        /// \param Vector The per-axis factors to multiply the current scale by. None may be zero.
        /// \return This pose, allowing for method chaining.
        ZY_INLINE Ref<Pose> Scale(Vector3 Vector)
        {
            ZY_ASSERT(Vector.GetX() != 0.0f, "Scale X must not be zero");
            ZY_ASSERT(Vector.GetY() != 0.0f, "Scale Y must not be zero");
            ZY_ASSERT(Vector.GetZ() != 0.0f, "Scale Z must not be zero");

            mScale *= Vector;
            return (* this);
        }

        /// \brief Rotates the entity by a rotation.
        ///
        /// \param Rotation The rotation to apply on top of the current one.
        /// \return This pose, allowing for method chaining.
        ZY_INLINE Ref<Pose> Rotate(Quaternion Rotation)
        {
            mRotation = Quaternion::Normalize(Rotation * mRotation);
            return (* this);
        }

        /// \brief Rotates the entity about an axis.
        ///
        /// \param Rotation The angle to turn by.
        /// \param Axis     The axis to turn about (must be normalized).
        /// \return This pose, allowing for method chaining.
        ZY_INLINE Ref<Pose> Rotate(Angle Rotation, Vector3 Axis)
        {
            ZY_ASSERT(Axis.IsNormalized(), "Rotation axis should be normalized");

            return Rotate(Quaternion::FromAngles(Rotation, Axis));
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Vector3    mTranslation;
        Vector3    mScale;
        Quaternion mRotation;
    };
}