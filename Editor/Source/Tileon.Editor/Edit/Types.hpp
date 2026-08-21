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

#include "Tileon.Editor/Context.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Defines the different commands that can be executed.
    enum class Command : UInt8
    {
        Add,        ///< A command used for adding tiles or objects to the scene.
        Remove,     ///< A command used for removing tiles or objects from the scene.
    };

    /// \brief Defines the different brush types that can be used for editing the scene.
    enum class Brush : UInt8
    {
        Hand,       ///< A brush used for panning the view.
        Select,     ///< A brush used for selecting objects or areas in the scene.
        Pencil,     ///< A brush used for painting individual tiles.
        Bucket,     ///< A brush used for filling an area with a specific tile type.
    };

    /// \brief Defines the different modes that can be used for editing the scene.
    enum class Mode : UInt8
    {
        Ground,     ///< A mode used for painting the terrain the ground blends between.
        Entity,     ///< A mode used for placing entities, decals among them, on top of it.
    };

    /// \brief Defines the footprint the ground brush covers.
    enum class Shape : UInt8
    {
        Square,     ///< The brush covers a square, which suits laying broad even ground down.
        Circle,     ///< The brush covers a disc, which leaves no corners for the eye to catch on.
        Diamond,    ///< The brush covers a diamond, which follows the way the units themselves sit.
    };
}