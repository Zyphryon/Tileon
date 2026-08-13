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
    /// \brief Represents a world-space transform with support for origin rebasing.
    class Transform final
    {
    public:

        /// \brief Constructs an identity transform at the world origin.
        ZY_INLINE Transform() = default;

        /// \brief Constructs a transform from a matrix and the origin it is relative to.
        ///
        /// \param Worldspace The transform matrix, relative to \p Origin.
        /// \param Origin     The whole-tile origin the matrix is expressed against.
        ZY_INLINE Transform(ConstRef<Matrix4x3> Worldspace, IntVector3 Origin)
            : mWorldspace { Worldspace },
              mOrigin     { Origin }
        {
        }

        /// \brief Constructs an identity transform against the given origin.
        ///
        /// \param Origin The whole-tile origin the transform is expressed against.
        ZY_INLINE Transform(IntVector3 Origin)
            : mWorldspace { Matrix4x3::Identity() },
              mOrigin     { Origin }
        {
        }

        /// \brief Re-expresses the transform against a different origin.
        ///
        /// \param Origin The origin to rebase onto, typically the one the camera is rendering from.
        /// \return The transform matrix as seen from \p Origin.
        ZY_INLINE Matrix4x3 Rebase(IntVector3 Origin) const
        {
            return Matrix4x3::WithTranslation(mWorldspace, Vector3(mOrigin - Origin));
        }

        /// \brief Sets the transform matrix.
        ///
        /// \param Worldspace The transform matrix, relative to the current origin.
        ZY_INLINE void SetWorldspace(ConstRef<Matrix4x3> Worldspace)
        {
            mWorldspace = Worldspace;
        }

        /// \brief Gets the transform matrix.
        ///
        /// \return The transform matrix, relative to the current origin.
        ZY_INLINE ConstRef<Matrix4x3> GetWorldspace() const
        {
            return mWorldspace;
        }

        /// \brief Sets the whole-tile origin the matrix is expressed against.
        ///
        /// \param Origin The origin, inherited from the owning region.
        ZY_INLINE void SetOrigin(IntVector3 Origin)
        {
            mOrigin = Origin;
        }

        /// \brief Gets the whole-tile origin the matrix is expressed against.
        ///
        /// \return The origin, inherited from the owning region.
        ZY_INLINE IntVector3 GetOrigin() const
        {
            return mOrigin;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Matrix4x3  mWorldspace;
        IntVector3 mOrigin;
    };
}