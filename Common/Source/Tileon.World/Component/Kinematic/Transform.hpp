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
    /// \brief Represents a world-space transform with support for origin rebasing.
    class Transform final
    {
    public:

        /// \brief Default constructor initializing the transform with an identity world-space matrix.
        ZY_INLINE Transform()
            : mWorldspace { Matrix3x2::Identity() },
              mOrigin     { 0, 0 }
        {
        }

        /// \brief Constructs a transform with the specified world-space transformation matrix and origin.
        ///
        /// \param Worldspace The world-space transformation matrix to initialize the transform with.
        /// \param Origin     The origin to initialize the transform with, used for rebasing the transformation.
        ZY_INLINE Transform(ConstRef<Matrix3x2> Worldspace, IntVector2 Origin)
            : mWorldspace { Worldspace },
              mOrigin     { Origin }
        {
        }

        /// \brief Rebases the world-space transformation matrix of the transform to a new origin.
        ///
        /// \param Origin The new origin to rebase the transformation to.
        /// \return A new world-space transformation matrix that is rebased to the specified origin.
        ZY_INLINE Matrix3x2 Rebase(IntVector2 Origin) const
        {
            return Matrix3x2::WithTranslation(mWorldspace, Vector2(mOrigin - Origin));
        }

        /// \brief Sets the world-space transformation matrix for the transform.
        ///
        /// \param Worldspace The world-space transformation matrix to set for the transform.
        ZY_INLINE void SetWorldspace(ConstRef<Matrix3x2> Worldspace)
        {
            mWorldspace = Worldspace;
        }

        /// \brief Gets the world-space transformation matrix of the transform.
        ///
        /// \return The world-space transformation matrix of the transform.
        ZY_INLINE ConstRef<Matrix3x2> GetWorldspace() const
        {
            return mWorldspace;
        }


        /// \brief Sets the origin of the transform.
        ///
        /// \param Origin The origin to set for the transform.
        ZY_INLINE void SetOrigin(IntVector2 Origin)
        {
            mOrigin = Origin;
        }

        /// \brief Gets the origin of the transform.
        ///
        /// \return The origin of the transform.
        ZY_INLINE IntVector2 GetOrigin() const
        {
            return mOrigin;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Matrix3x2  mWorldspace;
        IntVector2 mOrigin;
    };
}