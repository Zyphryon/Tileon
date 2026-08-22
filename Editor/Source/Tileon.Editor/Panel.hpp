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
#include "Toolkit/Composer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Represents an activity within the editor.
    class Panel : public Retainable<Panel>
    {
    public:

        /// \brief Constructs an activity with the specified title and visibility.
        ///
        /// \param Context The context associated with this activity.
        /// \param Title   The title of the activity.
        /// \param Visible `true` to make the activity visible, `false` to hide it. Defaults to `false`.
        ZY_INLINE Panel(Ref<Context> Context, Text Title, Bool Visible = false)
            : mContext { Context },
              mTitle   { Title },
              mVisible { Visible }
        {
        }

        /// \brief Destructor for the activity class.
        ZY_INLINE virtual ~Panel() = default;

        /// \brief Get the context associated with this activity.
        ///
        /// \return The context associated with this activity.
        ZY_INLINE Ref<Context> GetContext()
        {
            return mContext;
        }

        /// \brief Get the title of the activity.
        ///
        /// \return The current title of the activity.
        ZY_INLINE Text GetTitle() const
        {
            return mTitle;
        }

        /// \brief Set the visibility of the activity.
        ///
        /// \param Visible `true` to make the activity visible, `false` to hide it.
        ZY_INLINE void SetVisible(Bool Visible)
        {
            mVisible = Visible;
        }

        /// \brief Check if the activity is currently visible.
        ///
        /// \return `true` if the activity is visible, `false` otherwise.
        ZY_INLINE Bool IsVisible() const
        {
            return mVisible;
        }

        /// \brief Writes back whatever the panel has been holding until the project is saved.
        virtual void OnCommit()
        {
        }

        /// \brief Called when the panel is active and should perform its drawing operations.
        virtual void OnDraw()
        {
        }

    protected:

        /// \brief Draws a centred hint in place of the content the panel has none of.
        ///
        /// \param Message The hint to draw.
        /// \param Icon    The glyph drawn above the hint, or empty to draw the hint on its own.
        void DrawEmptyPanel(Text Message, Text Icon = Text());

        /// \brief Draws the bottom status bar hosting the given contents.
        ///
        /// \param ID      The identifier of the status bar child.
        /// \param Content The function drawing the bar, given the baseline its text sits on.
        template<typename Function>
        ZY_INLINE void DrawBottomBar(Text ID, AnyRef<Function> Content)
        {
            const Real32 BarHeight = Toolkit::Composer::GetFrameHeightWithSpacing() + 4.0f;

            Toolkit::Composer::PushStyleColor(ImGuiCol_ChildBg, Toolkit::Composer::GetStyleColorVec4(ImGuiCol_MenuBarBg));
            Toolkit::Composer::BeginChild(ID, ImVec2(0.0f, BarHeight), ImGuiChildFlags_None);
            Toolkit::Composer::PopStyleColor();

            // Vertically center text inside the bar.
            const Real32 PadY = (BarHeight - Toolkit::Composer::GetTextLineHeight()) * 0.5f
                - Toolkit::Composer::GetStyle().ItemSpacing.y * 0.5f;
            Toolkit::Composer::SetCursorPosY(PadY);

            Content(PadY);

            Toolkit::Composer::EndChild();
        }

    protected:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Context> mContext;
        Str          mTitle;
        Bool         mVisible;
    };
}