// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Tileon.Editor/Edit/Assembler.hpp"
#include "Tileon.Editor/Panel.hpp"
#include "Tileon.Editor/Toolkit/Previewer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Authors the terrains the ground is painted with, and everything they are dressed by.
    class Terrain final : public Panel
    {
    public:

        /// \brief Constructs the panel with the specified context.
        ///
        /// \param Context The context associated with this panel.
        Terrain(Ref<Context> Context);

        /// \see Panel::OnDraw()
        void OnDraw() override;

        /// \see Panel::OnCommit()
        void OnCommit() override;

    private:

        /// \brief Draws the list of terrains the project holds, and the fields that add another.
        void DrawListPanel();

        /// \brief Draws the art the selected terrain is baked from, one array to a tab.
        void DrawPreviewPanel();

        /// \brief Draws the status bar naming the selected terrain.
        void DrawStatusBar();

        /// \brief Draws one of the arts the selected terrain is baked from.
        ///
        /// \param Label  The name the slot carries.
        /// \param Slot   The art the field names.
        /// \param Hint   The tooltip the field carries.
        void DrawSlot(Text Label, Assembler::Slot Slot, Text Hint);

        /// \brief Draws the editable properties of the selected terrain.
        void DrawProperties();

        /// \brief Draws what the ground as a whole is set to.
        void DrawSettings();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Splatset>      mSplatset;
        Assembler          mAssembler;
        Toolkit::Previewer mPreview;
        Str                mPendingAlbedo;
        Str                mPendingNormal;
        Str                mPendingHeight;
        Str                mPendingName;
    };
}