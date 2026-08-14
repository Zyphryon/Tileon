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

#include <Zyphryon.Content/Service.hpp>
#include <imgui.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Toolkit
{
    /// \brief Provides functionality for applying different themes to the user interface.
    class Theme
    {
    public:

        /// What the selection owns, at three weights: outlines, the shapes hanging off them, and filled areas.
        static constexpr UInt32 kSelect      = IM_COL32(255, 170,  40, 225);
        static constexpr UInt32 kSelectSoft  = IM_COL32(255, 170,  40, 175);
        static constexpr UInt32 kSelectWash  = IM_COL32(255, 170,  40,  40);

        /// What the cursor is on or dragging, kept apart from a committed selection.
        static constexpr UInt32 kActive      = IM_COL32(255, 205,  70, 255);
        static constexpr UInt32 kActiveSoft  = IM_COL32(255, 205,  70, 150);

        /// What a click would take, lighter again so a hint never reads as a selection.
        static constexpr UInt32 kHint        = IM_COL32(255, 205,  70, 210);

        /// Readouts and markers that carry no meaning of their own, at three weights.
        static constexpr UInt32 kMarker      = IM_COL32(255, 255, 255, 235);
        static constexpr UInt32 kMarkerSoft  = IM_COL32(255, 255, 255, 180);
        static constexpr UInt32 kMarkerFaint = IM_COL32(255, 255, 255, 120);

        /// Where a brush would paint, off the selection's hue so the two never blur together.
        static constexpr UInt32 kBrush       = IM_COL32(120, 200, 255, 220);
        static constexpr UInt32 kBrushWash   = IM_COL32(120, 200, 255,  45);

        /// The world axes, which keep the usual red, green and blue reading of X, Y and Z.
        static constexpr UInt32 kAxisX       = IM_COL32(220,  70,  70, 255);
        static constexpr UInt32 kAxisY       = IM_COL32( 90, 200,  90, 255);
        static constexpr UInt32 kAxisZ       = IM_COL32( 90, 160, 235, 255);
        static constexpr UInt32 kAxisFree    = IM_COL32(200, 200, 200, 255);

        /// The chrome a widget draws its own face, rim and grid from.
        static constexpr UInt32 kFieldFace   = IM_COL32( 22,  26,  34, 255);
        static constexpr UInt32 kFieldEdge   = IM_COL32( 92, 102, 122, 255);
        static constexpr UInt32 kFieldGrid   = IM_COL32( 64,  72,  88, 255);
        static constexpr UInt32 kFieldHollow = IM_COL32( 40,  44,  54, 255);

        /// The ground behind everything, and the checker that stands in for transparency.
        static constexpr UInt32 kBackdrop    = IM_COL32( 18,  18,  28, 255);
        static constexpr UInt32 kCheckerDark = IM_COL32( 80,  80,  80, 255);
        static constexpr UInt32 kCheckerLite = IM_COL32(120, 120, 120, 255);

    public:

        /// \brief Initializes the theme system: loads fonts, configures ImGui IO flags, and applies the dark style.
        ///
        /// \param Content The content service the embedded fonts are read from.
        static void Initialize(Ref<Content::Service> Content);

        /// \brief Applies the dark theme to the user interface, setting colors and styles for a dark appearance.
        static void ApplyDarkStyle();

        /// \brief Applies the light theme to the user interface, setting colors and styles for a light appearance.
        static void ApplyLightStyle();
    };
}
