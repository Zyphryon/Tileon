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

#include <Zyphryon.Math/Pivot.hpp>
#include <Zyphryon.Math/Vector2.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a text component that contains runtime data for a text entity.
    class Text final
    {
    public:

        /// \brief Default constructor.
        ZYPHRYON_INLINE Text() = default;

        /// \brief Constructs a text component with the specific properties.
        ///
        /// \param Content The string content of the text.
        /// \param Spacing The spacing between characters in the text, in pixels.
        /// \param Pivot   The pivot point for text alignment.
        /// \param Effect  The rendering effect to apply to the text.
        ZYPHRYON_INLINE Text(ConstStr8 Content, Vector2 Spacing, Pivot Pivot, UInt8 Effect)
            : mContent { Content },
              mSpacing { Spacing },
              mPivot   { Pivot },
              mEffect  { Effect }
        {
        }

        /// \brief Sets the string content of the text.
        ///
        /// \param Content The string content to set for the text.
        ZYPHRYON_INLINE void SetContent(ConstStr8 Content)
        {
            mContent = Content;
        }

        /// \brief Gets the string content of the text.
        ///
        /// \return The string content of the text.
        ZYPHRYON_INLINE ConstStr8 GetContent() const
        {
            return mContent;
        }

        /// \brief Sets the spacing between characters in the text.
        ///
        /// \param Spacing The spacing to set between characters, in pixels.
        ZYPHRYON_INLINE void SetSpacing(Vector2 Spacing)
        {
            mSpacing = Spacing;
        }

        /// \brief Gets the spacing between characters in the text.
        ///
        /// \return The spacing between characters in the text, in pixels.
        ZYPHRYON_INLINE Vector2 GetSpacing() const
        {
            return mSpacing;
        }

        /// \brief Sets the pivot point for text alignment.
        ///
        /// \param Pivot The pivot point to set for text alignment.
        ZYPHRYON_INLINE void SetPivot(Pivot Pivot)
        {
            mPivot = Pivot;
        }

        /// \brief Gets the pivot point for text alignment.
        ///
        /// \return The pivot point for text alignment.
        ZYPHRYON_INLINE Pivot GetPivot() const
        {
            return mPivot;
        }

        /// \brief Sets the rendering effect to apply to the text.
        ///
        /// \param Effect The effect to set.
        ZYPHRYON_INLINE void SetEffect(UInt8 Effect)
        {
            mEffect = Effect;
        }

        /// \brief Gets the rendering effect applied to the text.
        ///
        /// \return The effect applied to the text.
        ZYPHRYON_INLINE UInt8 GetEffect() const
        {
            return mEffect;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
        {
            Archive.SerializeText(mContent);
            Archive.SerializeObject(mSpacing);
            Archive.SerializeObject(mPivot);
            Archive.SerializeUInt8(mEffect);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Str8    mContent;
        Vector2 mSpacing;
        Pivot   mPivot;
        UInt8   mEffect;
    };
}
