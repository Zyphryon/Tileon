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
#include "Tileon.Editor.UI/Widget/Gallery.hpp"
#include "Tileon.World/Repository.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    /// \brief Provides a palette interface for browsing and selecting terrains or archetypes in the editor.
    class Palette final : public Activity
    {
    public:

        /// \brief Constructs the activity with the specified context.
        ///
        /// \param Context The context associated with this activity.
        Palette(Ref<Context> Context);

        /// \see Activity::OnDraw(Ref<UI::Composer>)
        void OnDraw(Ref<UI::Composer> Composer) override;

    private:

        /// \brief Draws the tab listing the terrains available in the repository.
        ///
        /// \param Composer The UI composer used to render the tab.
        void DrawTerrainTab(Ref<UI::Composer> Composer);

        /// \brief Draws the tab listing the archetypes available in the repository.
        ///
        /// \param Composer The UI composer used to render the tab.
        void DrawEntityTab(Ref<UI::Composer> Composer);

        /// \brief Draws the gallery of terrains available in the repository.
        ///
        /// \param Composer The UI composer used to render the gallery.
        void DrawTerrainGallery(Ref<UI::Composer> Composer);

        /// \brief Draws the gallery of archetypes available in the repository.
        ///
        /// \param Composer The UI composer used to render the gallery.
        void DrawEntityGallery(Ref<UI::Composer> Composer);

        /// \brief Draws the bottom status bar hosting the given contents.
        ///
        /// \param Composer The UI composer used to render the status bar.
        /// \param ID       The identifier of the status bar child.
        /// \param Content  The function invoked to draw the contents of the bar.
        template<typename Function>
        ZY_INLINE void DrawBottomBar(Ref<UI::Composer> Composer, Text ID, AnyRef<Function> Content)
        {
            const Real32 BarHeight = Composer.GetFrameHeightWithSpacing() + 4.0f;

            Composer.PushStyleColor(ImGuiCol_ChildBg, Composer.GetStyleColorVec4(ImGuiCol_MenuBarBg));
            Composer.BeginChild(ID, ImVec2(0.0f, BarHeight), ImGuiChildFlags_None);
            Composer.PopStyleColor();

            // Vertically center text inside the bar.
            const Real32 PadY = (BarHeight - Composer.GetTextLineHeight()) * 0.5f
                - Composer.GetStyle().ItemSpacing.y * 0.5f;
            Composer.SetCursorPosY(PadY);

            Content();

            Composer.EndChild();
        }

        /// \brief Draws the status bar contents describing the selected terrain.
        ///
        /// \param Composer The UI composer used to render the status bar contents.
        void DrawTerrainStatus(Ref<UI::Composer> Composer);

        /// \brief Draws the status bar contents describing the selected archetype.
        ///
        /// \param Composer The UI composer used to render the status bar contents.
        void DrawEntityStatus(Ref<UI::Composer> Composer);

        /// \brief Draws a centered, dimmed hint inside the status bar.
        ///
        /// \param Composer The UI composer used to render the hint.
        /// \param Hint     The message to display.
        void DrawHint(Ref<UI::Composer> Composer, Text Hint);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Repository> mRepository;
        Ref<Tileset>    mTileset;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UI::Gallery     mTerrains;
        UI::Gallery     mEntities;
    };
}