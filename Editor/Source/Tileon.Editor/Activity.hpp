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

#include "Context.hpp"
#include "Tileon.Editor.UI/Composer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Represents an activity within the editor.
    class Activity : public Trackable<Activity>
    {
    public:

        /// \brief Constructs an activity with the specified title and visibility.
        ///
        /// \param Context The context associated with this activity.
        /// \param Title   The title of the activity.
        /// \param Visible `true` to make the activity visible, `false` to hide it. Defaults to `false`.
        ZYPHRYON_INLINE Activity(Ref<Context> Context, ConstStr8 Title, Bool Visible = false)
            : mContext { Context },
              mTitle   { Title },
              mVisible { Visible }
        {
        }

        /// \brief Destructor for the activity class.
        ZYPHRYON_INLINE virtual ~Activity() = default;

        /// \brief Get the context associated with this activity.
        ///
        /// \return The context associated with this activity.
        ZYPHRYON_INLINE Ref<Context> GetContext()
        {
            return mContext;
        }

        /// \brief Set the title of the activity.
        ///
        /// \param Title The new title for the activity.
        ZYPHRYON_INLINE void SetTitle(ConstStr8 Title)
        {
            mTitle = Title;
        }

        /// \brief Get the title of the activity.
        ///
        /// \return The current title of the activity.
        ZYPHRYON_INLINE ConstStr8 GetTitle() const
        {
            return mTitle;
        }

        /// \brief Set the visibility of the activity.
        ///
        /// \param Visible `true` to make the activity visible, `false` to hide it.
        ZYPHRYON_INLINE void SetVisible(Bool Visible)
        {
            mVisible = Visible;
        }

        /// \brief Check if the activity is currently visible.
        ///
        /// \return `true` if the activity is visible, `false` otherwise.
        ZYPHRYON_INLINE Bool IsVisible() const
        {
            return mVisible;
        }

        /// \brief Called when the activity is active and should perform its drawing operations.
        ///
        /// \param Composer The composer to use for drawing the activity's user interface.
        virtual void OnDraw(Ref<UI::Composer> Composer)
        {
        }

    protected:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Context> mContext;
        Str8         mTitle;
        Bool         mVisible;
    };
}