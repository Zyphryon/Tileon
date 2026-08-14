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

#include "Glaze.hpp"
#include "Tileon.Editor/Context.hpp"
#include "Tileon.Editor/Toolkit/Widget/Picker.hpp"
#include <Zyphryon.Graphic/Types.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Bakes an asset sitting anywhere on disk into a folder of the project.
    class Importer final
    {
    public:

        /// \brief Enumerates what an import in flight is waiting on.
        enum class Kind : UInt8
        {
            None,       ///< Nothing is being imported.
            Font,       ///< A typeface is waiting on its baking settings.
            Texture,    ///< An image is waiting on its baking settings.
        };

        /// \brief Constructs the importer with the specified context.
        ///
        /// \param Context The context associated with this importer.
        Importer(Ref<Context> Context);

        /// \brief Opens the browser that chooses the source of an import.
        ///
        /// \param Kind What the chosen source is to be baked into.
        void Browse(Kind Kind);

        /// \brief Draws the prompt an import in flight waits on, at the scope of the window it belongs to.
        ///
        /// \param Folder The folder the bake writes into.
        /// \return `true` if the bake wrote into the folder, `false` otherwise.
        Bool DrawPrompt(Text Folder);

        /// \brief Draws the browser that chooses the source, which is a window of its own.
        void DrawBrowser();

    private:

        /// \brief Draws the prompt that settles how a typeface is baked before it is imported.
        ///
        /// \param Folder The folder the bake writes into.
        /// \return `true` if the typeface was baked, `false` otherwise.
        Bool DrawFontPrompt(Text Folder);

        /// \brief Draws the prompt that settles how an image is baked before it is imported.
        ///
        /// \param Folder The folder the bake writes into.
        /// \return `true` if the image was baked, `false` otherwise.
        Bool DrawTexturePrompt(Text Folder);

        /// \brief Bakes a typeface into the specified folder.
        ///
        /// \param Source The typeface to import, which is left where it is.
        /// \param Folder The folder the bake writes into.
        /// \return `true` if the typeface was baked, `false` otherwise.
        Bool ImportFont(Text Source, Text Folder);

        /// \brief Bakes an image into the specified folder, and names the texture from a material.
        ///
        /// \param Source The image to import, which is left where it is.
        /// \param Folder The folder the bake writes into.
        /// \return `true` if the image was baked, `false` otherwise.
        Bool ImportTexture(Text Source, Text Folder);

        /// \brief Writes the material that names a baked texture, and its normal map when one came along.
        ///
        /// \param Folder The folder the material is written into.
        /// \param Stem   The file name the material and its textures share.
        /// \param Normal Whether a normal map was baked beside the albedo.
        void WriteMaterial(Text Folder, Text Stem, Bool Normal);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Context>           mContext;
        Toolkit::Picker        mPicker;
        Kind                   mPending;
        Str                    mImport;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Str                    mFontCharset;
        Real32                 mFontSize;
        Real32                 mFontRange;
        Real32                 mFontUnderline;
        UInt32                 mFontPadding;
        UInt32                 mFontLimit;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Graphic::TextureFormat mTextureFormat;
        Bool                   mTextureMaterial;
        Bool                   mTextureMipmaps;
        Bool                   mTextureLinear;
    };
}