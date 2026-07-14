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

#include <Zyphryon.Math/Pivot2D.hpp>
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
        ZY_INLINE Text() = default;

        /// \brief Constructs a text component with the specific properties.
        ///
        /// \param Content The string content of the text.
        /// \param Spacing The spacing between characters in the text, in pixels.
        /// \param Pivot   The pivot point for text alignment.
        ZY_INLINE Text(Base::Text Content, Vector2 Spacing, Pivot2D Pivot)
            : mContent { Content },
              mSpacing { Spacing },
              mPivot   { Pivot }
        {
        }

        /// \brief Sets the string content of the text.
        ///
        /// \param Content The string content to set for the text.
        ZY_INLINE void SetContent(Base::Text Content)
        {
            mContent = Content;
        }

        /// \brief Gets the string content of the text.
        ///
        /// \return The string content of the text.
        ZY_INLINE Base::Text GetContent() const
        {
            return mContent;
        }

        /// \brief Sets the spacing between characters in the text.
        ///
        /// \param Spacing The spacing to set between characters, in pixels.
        ZY_INLINE void SetSpacing(Vector2 Spacing)
        {
            mSpacing = Spacing;
        }

        /// \brief Gets the spacing between characters in the text.
        ///
        /// \return The spacing between characters in the text, in pixels.
        ZY_INLINE Vector2 GetSpacing() const
        {
            return mSpacing;
        }

        /// \brief Sets the pivot point for text alignment.
        ///
        /// \param Pivot The pivot point to set for text alignment.
        ZY_INLINE void SetPivot(Pivot2D Pivot)
        {
            mPivot = Pivot;
        }

        /// \brief Gets the pivot point for text alignment.
        ///
        /// \return The pivot point for text alignment.
        ZY_INLINE Pivot2D GetPivot() const
        {
            return mPivot;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZY_INLINE void Serialize(Serializer Archive)
        {
            Archive.Serialize(mContent);
            Archive.Serialize(mSpacing);
            Archive.Serialize(mPivot);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Str     mContent;
        Vector2 mSpacing;
        Pivot2D mPivot;
    };
}
