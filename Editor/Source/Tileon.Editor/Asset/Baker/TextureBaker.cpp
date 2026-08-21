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

#include "TextureBaker.hpp"
#include <Tileon.Render/Types.hpp>
#include "Tileon.Editor/Asset/Editor/MaterialEditor.hpp"
#include "Tileon.Editor/Toolkit/Composer.hpp"
#include <Baker.Texture/Baker.hpp>
#include <Baker.Texture/Exporter.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    TextureBaker::TextureBaker(Ref<Context> Context)
        : mContext  { Context },
          mFormat   { Graphic::TextureFormat::Unspecified },
          mMaterial { true },
          mMipmaps  { true },
          mLinear   { false }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void TextureBaker::DrawSettings()
    {
        Toolkit::Composer::FieldInline("Format");

        if (Toolkit::Composer::BeginCombo("##Format", Enum::GetName(mFormat)))
        {
            for (const Graphic::TextureFormat Format : Enum::GetValues<Graphic::TextureFormat>())
            {
                if (!Pipeline::Baker::Texture::Exporter::IsSupported(Format))
                {
                    continue;
                }

                if (Toolkit::Composer::Selectable(Enum::GetName(Format), mFormat == Format))
                {
                    mFormat = Format;
                }
            }
            Toolkit::Composer::EndCombo();
        }
        Toolkit::Composer::Tooltip("Unspecified lets the baker choose from the source and the options below."_Text);

        Toolkit::Composer::Checkbox("Mipmaps", mMipmaps);

        // Naming a format has already said whether the art is sRGB, since sRGB is a format rather than a
        // flag, so the toggle follows it instead of contradicting it.
        const Bool Named = (mFormat != Graphic::TextureFormat::Unspecified);

        if (Named)
        {
            mLinear = !StrEndsWith(Enum::GetName(mFormat), "_sRGB");
        }

        Toolkit::Composer::BeginDisabled(Named);
        Toolkit::Composer::Checkbox("Linear", mLinear);
        Toolkit::Composer::EndDisabled();
        Toolkit::Composer::Tooltip(Named
            ? "Follows the format above, the only place sRGB is expressed."_Text
            : "Off for art, which is authored in sRGB. On for data such as a normal map."_Text);

        Toolkit::Composer::Checkbox("Create Material", mMaterial);
        Toolkit::Composer::Tooltip("Writes the .mtl that names the baked texture, and a _n sibling as its normal."_Text);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool TextureBaker::Bake(Text Source, Text Folder)
    {
        const Text File = StrAfterLast(Source, '/');
        const Text Stem = StrBeforeLast(File, '.');
        const Text Type = StrAfterLast(File, '.');

        const Str Baked = Str::Print<"{0}/{1}.tex">(Folder, Stem);

        Pipeline::Baker::Texture::Profile Settings;
        Settings.Format   = mFormat;
        Settings.Mipmaps  = mMipmaps;
        Settings.Linear   = mLinear;
        Settings.Compress = true;

        const Pipeline::Baker::Texture::Baker Baker(mContext.GetScheduler());

        if (!Baker.Bake(Source, Baked, Settings))
        {
            LOG_E("TextureBaker: failed to bake '{0}'", Baked);
            return false;
        }

        if (mMaterial)
        {
            WriteMaterial(Folder, Stem);
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void TextureBaker::WriteMaterial(Text Folder, Text Stem)
    {
        Sequence<MaterialEditor::Binding> Bindings;

        Ref<MaterialEditor::Binding> Albedo = Bindings.Append();
        Albedo.Name = Enum::GetName(Texture::Albedo);
        Albedo.Path = Str::Print<"{0}.tex">(Stem);

        MaterialEditor::Write(Str::Print<"{0}/{1}.mtl">(Folder, Stem), Bindings);
    }
}