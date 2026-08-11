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

#include "Tileon.Editor/Activity.hpp"
#include "Tileon.Editor/Toolkit/Widget/Gallery.hpp"
#include "Tileon.World/Repository.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Provides a palette interface for browsing and selecting terrains or archetypes in the editor.
    class Palette final : public Activity
    {
    public:

        /// \brief Constructs the activity with the specified context.
        ///
        /// \param Context The context associated with this activity.
        Palette(Ref<Context> Context);

        /// \see Activity::OnDraw()
        void OnDraw() override;

    private:

        /// \brief Draws the tab listing the terrains available in the repository.
        void DrawTerrainTab();

        /// \brief Draws the tab listing the archetypes available in the repository.
        void DrawEntityTab();

        /// \brief Draws the gallery of terrains available in the repository.
        void DrawTerrainGallery();

        /// \brief Draws the gallery of archetypes available in the repository.
        void DrawEntityGallery();

        /// \brief Draws the bottom status bar hosting the given contents.
        ///
        /// \param ID      The identifier of the status bar child.
        /// \param Content The function invoked to draw the contents of the bar.
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

            Content();

            Toolkit::Composer::EndChild();
        }

        /// \brief Draws the status bar contents describing the selected terrain.
        void DrawTerrainStatus();

        /// \brief Draws the status bar contents describing the selected archetype.
        void DrawEntityStatus();

        /// \brief Draws a centered, dimmed hint inside the status bar.
        ///
        /// \param Hint The message to display.
        void DrawHint(Text Hint);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Repository>  mRepository;
        Ref<Tileset>     mTileset;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Toolkit::Gallery mTerrains;
        Toolkit::Gallery mEntities;
        SInt32           mMode;
    };
}