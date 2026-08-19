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

#include "Tileon.Editor/Toolkit/Previewer.hpp"
#include <Zyphryon.Graphic/Material.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Draws a baked texture.
    ///
    /// \param Preview The previewer the image is drawn through, which holds the zoom, pan and slice.
    /// \param Asset   The texture to describe, which may still be loading.
    void Inspect(Ref<Toolkit::Previewer> Preview, ConstRetainer<Graphic::Image> Asset);

    /// \brief Draws a baked material.
    ///
    /// \param Preview The previewer the bound art is drawn through.
    /// \param Asset   The material to describe, which may still be loading.
    void Inspect(Ref<Toolkit::Previewer> Preview, ConstRetainer<Graphic::Material> Asset);

    /// \brief Draws a baked texture at a fixed size, captioned with what it is.
    ///
    /// \param Asset The texture to show, which may still be loading.
    /// \param Size  The longest side the drawing may take, in pixels.
    /// \param Slice The slice to show, clamped to what the array holds.
    /// \return How many slices can be stepped through, or zero when there was nothing to show.
    UInt32 Thumbnail(ConstRetainer<Graphic::Image> Asset, Real32 Size, UInt32 Slice);

    /// \brief Draws one of the textures a material binds, at a fixed size.
    ///
    /// \param Asset   The material to show, which may still be loading.
    /// \param Size    The longest side the drawing may take, in pixels.
    /// \param Binding Which of the bound textures to show, clamped to how many there are.
    /// \return How many textures can be stepped through, or zero when there was nothing to show.
    UInt32 Thumbnail(ConstRetainer<Graphic::Material> Asset, Real32 Size, UInt32 Binding);
}