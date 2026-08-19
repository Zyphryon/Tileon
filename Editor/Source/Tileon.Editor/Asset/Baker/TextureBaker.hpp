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
#include "Tileon.Editor/Asset/AssetBaker.hpp"
#include <Zyphryon.Graphic/Types.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Bakes art into the native texture the renderer samples, and names it from a material.
    class TextureBaker final : public AssetBaker
    {
    public:

        /// \brief Constructs the baker with the specified context.
        ///
        /// \param Context The context associated with this baker.
        explicit TextureBaker(Ref<Context> Context);

        /// \see AssetBaker::GetSources()
        ZY_INLINE Text GetSources() const override
        {
            return ".png .jpg .jpeg .tga .bmp .hdr";
        }

        /// \see AssetBaker::GetLabel()
        ZY_INLINE Text GetLabel() const override
        {
            return "Texture";
        }

        /// \see AssetBaker::DrawSettings()
        void DrawSettings() override;

        /// \see AssetBaker::Bake(Text, Text)
        Bool Bake(Text Source, Text Folder) override;

    private:

        /// \brief Writes the material that names a baked texture, and its normal map when one came along.
        ///
        /// \param Folder The folder the material is written into.
        /// \param Stem   The file name the material and its textures share.
        static void WriteMaterial(Text Folder, Text Stem);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Context>           mContext;
        Graphic::TextureFormat mFormat;
        Bool                   mMaterial;
        Bool                   mMipmaps;
        Bool                   mLinear;
    };
}