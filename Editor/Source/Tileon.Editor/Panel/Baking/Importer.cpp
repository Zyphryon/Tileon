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

#include "Importer.hpp"
#include "Materializer.hpp"
#include <Baker.Font/Baker.hpp>
#include <Baker.Texture/Baker.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Importer::Importer(Ref<Context> Context)
        : mContext         { Context },
          mPending         { Kind::None },
          mFontCharset     { "ascii" },
          mFontSize        { 40.0f },
          mFontRange       { 20.0f },
          mFontUnderline   { 1.2f },
          mFontPadding     { 1 },
          mFontLimit       { 2048 },
          mTextureFormat   { Graphic::TextureFormat::Unspecified },
          mTextureMaterial { true },
          mTextureMipmaps  { true },
          mTextureLinear   { false }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Importer::Browse(Kind Kind)
    {
        const Text Filter = (Kind == Kind::Font) ? ".ttf"_Text : Text();

        mExplorer.Open(Explorer::Mode::Open, Filesystem::GetRootFolder(), Filter, [this, Kind](Text Path)
        {
            mImport  = Str(Path);
            mPending = Kind;
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Importer::DrawPrompt(Text Folder)
    {
        switch (mPending)
        {
        case Kind::Font:
            return DrawFontPrompt(Folder);
        case Kind::Texture:
            return DrawTexturePrompt(Folder);
        default:
            return false;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Importer::DrawExplorer()
    {
        mExplorer.Draw();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Importer::DrawFontPrompt(Text Folder)
    {
        Toolkit::Composer::OpenPopup("Import Font");

        if (!Toolkit::Composer::BeginPopupModal("Import Font"))
        {
            return false;
        }

        Toolkit::Composer::TextDisabled(StrAfterLast(mImport, '/'));
        Toolkit::Composer::Separator();

        Toolkit::Composer::FieldInline("Charset");
        Toolkit::Composer::InputText("##Charset", mFontCharset, [this](Text Value)
        {
            mFontCharset = Value;
        });
        Toolkit::Composer::Tooltip("ascii, latin1, punctuation, or spans such as 0x20-0x7E"_Text);

        Toolkit::Composer::FieldInline("Size");
        Toolkit::Composer::DragFloat("##Size", mFontSize, 1.0f);

        Toolkit::Composer::FieldInline("Range");
        Toolkit::Composer::DragFloat("##Range", mFontRange, 1.0f);

        Toolkit::Composer::FieldInline("Underline");
        Toolkit::Composer::DragFloat("##Underline", mFontUnderline, 0.01f);
        Toolkit::Composer::Tooltip("The vertical space an underline occupies, in em units."_Text);

        Toolkit::Composer::FieldInline("Padding");
        Toolkit::Composer::InputInt("##Padding", mFontPadding);
        Toolkit::Composer::Tooltip("The gap left between neighbouring glyphs in the atlas, in texels."_Text);

        Toolkit::Composer::FieldInline("Limit");
        if (Toolkit::Composer::BeginCombo("##Limit", String<16>::Print<"{0}">(mFontLimit)))
        {
            constexpr auto kLimits = Array(2048, 4096, 8192, 16384);

            for (const UInt32 Side : kLimits)
            {
                if (Toolkit::Composer::Selectable(String<16>::Print<"{0}">(Side), mFontLimit == Side))
                {
                    mFontLimit = Side;
                }
            }
            Toolkit::Composer::EndCombo();
        }
        Toolkit::Composer::Tooltip("The largest atlas side, in texels. A bake needing more opens another page."_Text);

        Toolkit::Composer::Separator();

        Bool Baked = false;

        if (Toolkit::Composer::Button("Import", 96.0f))
        {
            Baked = ImportFont(mImport, Folder);

            mPending = Kind::None;
            Toolkit::Composer::CloseCurrentPopup();
        }

        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::Button("Cancel", 96.0f))
        {
            mPending = Kind::None;
            Toolkit::Composer::CloseCurrentPopup();
        }

        Toolkit::Composer::EndPopup();

        return Baked;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Importer::DrawTexturePrompt(Text Folder)
    {
        Toolkit::Composer::OpenPopup("Import Texture");

        if (!Toolkit::Composer::BeginPopupModal("Import Texture"))
        {
            return false;
        }

        Toolkit::Composer::TextDisabled(StrAfterLast(mImport, '/'));
        Toolkit::Composer::Separator();

        Toolkit::Composer::FieldInline("Format");
        Toolkit::Composer::Combo("##Format", mTextureFormat);
        Toolkit::Composer::Tooltip("Unspecified lets the baker choose from the source and the options below."_Text);

        Toolkit::Composer::Checkbox("Mipmaps", mTextureMipmaps);

        Toolkit::Composer::Checkbox("Linear", mTextureLinear);
        Toolkit::Composer::Tooltip("Off for art, which is authored in sRGB. On for data such as a normal map."_Text);

        Toolkit::Composer::Checkbox("Create Material", mTextureMaterial);
        Toolkit::Composer::Tooltip("Writes the .mtl that names the baked texture, and a _n sibling as its normal."_Text);

        Toolkit::Composer::Separator();

        Bool Baked = false;

        if (Toolkit::Composer::Button("Import", 96.0f))
        {
            Baked = ImportTexture(mImport, Folder);

            mPending = Kind::None;
            Toolkit::Composer::CloseCurrentPopup();
        }

        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::Button("Cancel", 96.0f))
        {
            mPending = Kind::None;
            Toolkit::Composer::CloseCurrentPopup();
        }

        Toolkit::Composer::EndPopup();

        return Baked;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Importer::ImportFont(Text Source, Text Folder)
    {
        const Text Stem  = StrBeforeLast(StrAfterLast(Source, '/'), '.');
        const Str  Baked = Str::Print<"{0}/{1}.fnt">(Folder, Stem);

        ::Pipeline::Baker::Font::Profile Settings;
        Settings.Size      = mFontSize;
        Settings.Range     = mFontRange;
        Settings.Underline = mFontUnderline;
        Settings.Padding   = mFontPadding;
        Settings.Limit     = mFontLimit;

        if (!::Pipeline::Baker::Font::Profile::Parse(mFontCharset, Settings.Charset))
        {
            LOG_E("Importer: '{0}' is not a charset the baker understands", mFontCharset);
            return false;
        }

        const ::Pipeline::Baker::Font::Baker ComponentList(mContext.GetScheduler());

        if (!ComponentList.Bake(Source, Baked, Settings))
        {
            LOG_E("Importer: failed to bake '{0}'", Baked);
            return false;
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Importer::ImportTexture(Text Source, Text Folder)
    {
        const Text File = StrAfterLast(Source, '/');
        const Text Stem = StrBeforeLast(File, '.');
        const Text Type = StrAfterLast(File, '.');

        const Str Baked = Str::Print<"{0}/{1}.tex">(Folder, Stem);

        ::Pipeline::Baker::Texture::Profile Settings;
        Settings.Format   = mTextureFormat;
        Settings.Mipmaps  = mTextureMipmaps;
        Settings.Linear   = mTextureLinear;
        Settings.Compress = true;

        const ::Pipeline::Baker::Texture::Baker ComponentList(mContext.GetScheduler());

        if (!ComponentList.Bake(Source, Baked, Settings))
        {
            LOG_E("Importer: failed to bake '{0}'", Baked);
            return false;
        }

        // A normal map is named after the art it belongs to, so the pair is imported together or not at all.
        const Str Sibling = Str::Print<"{0}_n.{1}">(StrBeforeLast(Source, '.'), Type);

        Filesystem::Handle Handle;
        const Bool Normal = (Filesystem::Open(Sibling, Filesystem::Access::Read, Handle) == Filesystem::Result::Success);

        if (Normal)
        {
            Filesystem::Close(Handle);

            // A normal map carries direction rather than color, so it is never treated as sRGB.
            Settings.Linear = true;

            ComponentList.Bake(Sibling, Str::Print<"{0}/{1}_n.tex">(Folder, Stem), Settings);
        }

        if (mTextureMaterial)
        {
            WriteMaterial(Folder, Stem, Normal);
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Importer::WriteMaterial(Text Folder, Text Stem, Bool Normal)
    {
        Sequence<Materializer::Binding> Bindings;

        Materializer::Binding Albedo = Bindings.Append();
        Albedo.Name = "Albedo";
        Albedo.Path = Str::Print<"{0}.tex">(Stem);

        if (Normal)
        {
            Materializer::Binding Direction = Bindings.Append();
            Direction.Name = "Normal";
            Direction.Path = Str::Print<"{0}_n.tex">(Stem);
        }

        Materializer::Write(Str::Print<"{0}/{1}.mtl">(Folder, Stem), Bindings);
    }
}