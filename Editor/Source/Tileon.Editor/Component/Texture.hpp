// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Names the textures a material may bind for Tileon techniques.
    enum class Texture : UInt8
    {
        Albedo, ///< Base color map, sampled by the sprite, model and font techniques.
        Normal, ///< Tangent-space normal map, sampled by the sprite techniques that light their surface.
    };

    /// \brief Gets the hash a technique's reflection stores for a texture name.
    ///
    /// \param Slot The texture to resolve.
    /// \return The hash of the texture's name.
    ZY_INLINE constexpr UInt64 GetTextureID(Texture Slot)
    {
        switch (Slot)
        {
        case Texture::Albedo:
            return "Albedo"_Hash;
        case Texture::Normal:
            return "Normal"_Hash;
        default:
            return 0;
        }
    }
}