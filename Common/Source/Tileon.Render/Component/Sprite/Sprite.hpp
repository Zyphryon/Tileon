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
#include <Zyphryon.Math/Geometry/Rect.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a sprite component that contains archetypal data for rendering sprites.
    class Sprite final
    {
    public:

        /// \brief Default constructor.
        ZY_INLINE Sprite() = default;

        /// \brief Constructs a sprite with the specified path and source rectangle.
        ///
        /// \param Path   The path to the sprite's texture resource.
        /// \param Source The source rectangle for the sprite.
        ZY_INLINE Sprite(AnyRef<Content::Uri> Path, Rect Source = Rect::One())
            : mPath   { Move(Path) },
              mSource { Source }
        {
        }

        /// \brief Sets the path to the sprite's texture resource.
        ///
        /// \param Path The new path to set for the sprite's texture resource.
        ZY_INLINE void SetPath(AnyRef<Content::Uri> Path)
        {
            mPath = Move(Path);
        }

        /// \brief Gets the path to the sprite's texture resource.
        ///
        /// \return The path to the sprite's texture resource.
        ZY_INLINE ConstRef<Content::Uri> GetPath() const
        {
            return mPath;
        }

        /// \brief Sets the source rectangle for the sprite.
        ///
        /// \param Source The new source rectangle to set for the sprite.
        ZY_INLINE void SetSource(Rect Source)
        {
            mSource = Source;
        }

        /// \brief Gets the source rectangle for the sprite.
        ///
        /// \return The source rectangle for the sprite.
        ZY_INLINE Rect GetSource() const
        {
            return mSource;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZY_INLINE void Serialize(Serializer Archive)
        {
            Archive.Serialize(mPath);
            Archive.Serialize(mSource);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Content::Uri mPath;
        Rect         mSource;
    };
}