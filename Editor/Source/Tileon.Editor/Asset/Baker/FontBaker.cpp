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

#include "FontBaker.hpp"
#include "Tileon.Editor/Toolkit/Composer.hpp"
#include <Baker.Font/Baker.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    FontBaker::FontBaker(Ref<Context> Context)
        : mContext   { Context },
          mCharset   { "ascii" },
          mSize      { 40.0f },
          mRange     { 20.0f },
          mUnderline { 1.2f },
          mPadding   { 1 },
          mLimit     { 2048 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void FontBaker::DrawSettings()
    {
        Toolkit::Composer::FieldInline("Charset");
        Toolkit::Composer::InputText("##Charset", mCharset, [this](Text Value)
        {
            mCharset = Value;
        });
        Toolkit::Composer::Tooltip("ascii, latin1, punctuation, or spans such as 0x20-0x7E"_Text);

        Toolkit::Composer::FieldInline("Size");
        Toolkit::Composer::DragFloat("##Size", mSize, 1.0f);

        Toolkit::Composer::FieldInline("Range");
        Toolkit::Composer::DragFloat("##Range", mRange, 1.0f);

        Toolkit::Composer::FieldInline("Underline");
        Toolkit::Composer::DragFloat("##Underline", mUnderline, 0.01f);
        Toolkit::Composer::Tooltip("The vertical space an underline occupies, in em units."_Text);

        Toolkit::Composer::FieldInline("Padding");
        Toolkit::Composer::InputInt("##Padding", mPadding);
        Toolkit::Composer::Tooltip("The gap left between neighbouring glyphs in the atlas, in texels."_Text);

        Toolkit::Composer::FieldInline("Limit");

        if (Toolkit::Composer::BeginCombo("##Limit", String<16>::Print<"{0}">(mLimit)))
        {
            constexpr auto kLimits = Array(2048, 4096, 8192, 16384);

            for (const UInt32 Side : kLimits)
            {
                if (Toolkit::Composer::Selectable(String<16>::Print<"{0}">(Side), mLimit == Side))
                {
                    mLimit = Side;
                }
            }
            Toolkit::Composer::EndCombo();
        }
        Toolkit::Composer::Tooltip("The largest atlas side, in texels. A bake needing more opens another page."_Text);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool FontBaker::Bake(Text Source, Text Folder)
    {
        const Text Stem  = StrBeforeLast(StrAfterLast(Source, '/'), '.');
        const Str  Baked = Str::Print<"{0}/{1}.fnt">(Folder, Stem);

        Pipeline::Baker::Font::Profile Profile;
        Profile.Size      = mSize;
        Profile.Range     = mRange;
        Profile.Underline = mUnderline;
        Profile.Padding   = mPadding;
        Profile.Limit     = mLimit;

        if (!Pipeline::Baker::Font::Profile::Parse(mCharset, Profile.Charset))
        {
            LOG_E("FontBaker: '{0}' is not a charset the baker understands", mCharset);
            return false;
        }

        const Pipeline::Baker::Font::Baker Baker(mContext.GetScheduler());

        if (!Baker.Bake(Source, Baked, Profile))
        {
            LOG_E("FontBaker: failed to bake '{0}'", Baked);
            return false;
        }
        return true;
    }
}