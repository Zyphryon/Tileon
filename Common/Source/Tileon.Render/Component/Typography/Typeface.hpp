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

#include <Canvas/Typography/Font.hpp>
#include <Zyphryon.Content/Proxy.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a typeface component that contains archetypal font data for rendering text.
    class Typeface final
    {
    public:

        /// \brief Constructs a default typeface with no associated font resource.
        ZY_INLINE Typeface()
            : mSize { 0.0f }
        {
        }

        /// \brief Constructs a typeface with the specified font resource and size.
        ///
        /// \param Path The path to the font resource to associate with the typeface.
        /// \param Size The size to set for the typeface.
        ZY_INLINE Typeface(AnyRef<Content::Uri> Path, Real32 Size)
            : mFont { Move(Path) },
              mSize { Size }
        {
        }

        /// \brief Sets the font resource associated with the typeface.
        ///
        /// \param Path The path to the font resource to associate with the typeface.
        ZY_INLINE void SetFont(AnyRef<Content::Uri> Path)
        {
            mFont = Content::Proxy<Render::Font>(Move(Path));
        }

        /// \brief Gets the font resource associated with the typeface.
        ///
        /// \return The font resource associated with the typeface.
        ZY_INLINE ConstRetainer<Render::Font> GetFont() const
        {
            return mFont.GetResource();
        }

        /// \brief Sets the size of the typeface.
        ///
        /// \param Size The size to set for the typeface.
        ZY_INLINE void SetSize(Real32 Size)
        {
            mSize = Size;
        }

        /// \brief Gets the size of the typeface.
        ///
        /// \return The size of the typeface.
        ZY_INLINE Real32 GetSize() const
        {
            return mSize;
        }

        /// \brief Resolves the deferred resources of the object using the provided service.
        ///
        /// \param Service The service used to load the object resources.
        ZY_INLINE void OnResolve(Ref<Content::Service> Service)
        {
            mFont.Resolve(Service);
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZY_INLINE void Serialize(Serializer Archive)
        {
            Archive.Serialize(mFont);
            Archive.Serialize(mSize);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Content::Proxy<Render::Font> mFont;
        Real32                       mSize;
    };
}
