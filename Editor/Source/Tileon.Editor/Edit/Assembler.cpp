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

#include "Assembler.hpp"
#include "Tileon.Editor/Asset/Editor/MaterialEditor.hpp"
#include <Baker.Texture/Baker.hpp>
#include <Baker.Texture/Process/Mipmapper.hpp>
#include <Baker.Texture/Process/Transcoder.hpp>
#include <Zyphryon.Graphic/Metadata.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Assembler::Assembler(Ref<Context> Context, Ref<Splatset> Splatset)
        : mContext  { Context },
          mSplatset { Splatset }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Retainer<Graphic::Image> Assembler::GetArray(Texture Usage) const
    {
        ConstRetainer<Graphic::Material> Material = mSplatset.GetMaterial();
        return (Material && Material->HasCompleted()) ? Material->GetImage(GetTextureID(Usage)) : nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool Measure(
        ConstRef<Pipeline::Baker::Texture::Baker>   Baker,
        Text                                        Source,
        ConstRef<Pipeline::Baker::Texture::Profile> Profile,
        Ref<UInt16>                                 Width,
        Ref<UInt16>                                 Height)
    {
        const ConstPtr<Pipeline::Baker::Texture::Importer> Codec = Baker.Find(StrAfterLast(Source, '.'));

        Blob Input;

        if (Codec == nullptr || Filesystem::Read(Source, Input) != Filesystem::Result::Success)
        {
            LOG_E("Splatset: nothing here can read '{0}'", Source);

            return false;
        }

        const Pipeline::Baker::Texture::Surface Decoded = Codec->Import(Input, Profile);

        if (!Decoded.IsValid())
        {
            LOG_E("Splatset: '{0}' decoded to nothing", Source);

            return false;
        }

        Width  = Decoded.Slices.GetFront().GetWidth();
        Height = Decoded.Slices.GetFront().GetHeight();
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Assembler::Append(Text Albedo, Text Normal, Text Name)
    {
        using namespace Pipeline::Baker::Texture;

        const Str AlbedoArray = Resolve(Splatset::kAlbedo);
        const Str NormalArray = Resolve(Splatset::kNormal);

        // A project that has never had a tileset has nowhere to write one yet.
        Filesystem::MakeAll(StrBeforeLast(AlbedoArray, '/'));

        const Baker Baker(mContext.GetScheduler());

        // Arrays that already exist are grown one slice at a time and written in place, so only the art
        // being added is decoded and filtered. The very first terrain has no array to grow, and no extent
        // or format to match, so that one is still assembled onto disk to bring both into being.
        if (const Str Relief = (Normal.IsEmpty() ? Str() : Resolve(Normal)); AppendInPlace(Resolve(Albedo), Relief, Name))
        {
            return true;
        }

        // One array grows per pass: the colour always, the relief only when the terrain brought one.
        const auto Grow = [&](Text Source, Text Array, Text Mount, ConstRetainer<Graphic::Image> Existing, Bool Linear)
        {
            // Only an array that really loaded has slices worth keeping.
            const Bool Mounted = Existing && Existing->GetLayers() > 0;

            // Existing slices are re-read from the array the assemble is about to write over, so a material
            // pointing somewhere else would gather the wrong art and renumber every terrain already painted.
            if (Mounted && Existing->GetKey().GetUrl() != Mount)
            {
                LOG_E("Splatset: '{0}' binds '{1}' rather than '{2}', so it cannot be appended to",
                    Splatset::kMaterial, Existing->GetKey().GetUrl(), Mount);

                return false;
            }

            Profile Settings;
            Settings.Linear   = Linear;
            Settings.Mipmaps  = true;
            Settings.Compress = true;

            UInt16 Width  = Mounted ? Existing->GetWidth()  : 0;
            UInt16 Height = Mounted ? Existing->GetHeight() : 0;

            // An empty array has no extent to match, so the first terrain sets it and the source is decoded
            // once up front to say what it is.
            if (!Mounted && !Measure(Baker, Source, Settings, Width, Height))
            {
                return false;
            }

            // Every entry has to declare the same extent or the assemble is refused outright, and the
            // existing slices are re-read from the array itself so they keep the place they already hold.
            Sequence<Manifest::Entry> Entries;

            for (UInt16 Slice = 0; Mounted && Slice < Existing->GetLayers(); ++Slice)
            {
                Ref<Manifest::Entry> Entry = Entries.Append();
                Entry.Source = Array;
                Entry.Slice  = Slice;
                Entry.Width  = Width;
                Entry.Height = Height;
            }

            Ref<Manifest::Entry> Added = Entries.Append();
            Added.Source = Source;
            Added.Width  = Width;
            Added.Height = Height;

            Blob Baked = Baker.Assemble(Entries, Settings);
            return Baked && Filesystem::Write(Array, Baked) == Filesystem::Result::Success;
        };

        if (!Grow(Resolve(Albedo), AlbedoArray, Splatset::kAlbedo, GetArray(Texture::Albedo), false))
        {
            LOG_E("Splatset: failed to add '{0}' to '{1}'", Albedo, AlbedoArray);
            return false;
        }

        if (!Normal.IsEmpty() && !Grow(Resolve(Normal), NormalArray, Splatset::kNormal, GetArray(Texture::Normal), true))
        {
            LOG_E("Splatset: failed to add '{0}' to '{1}'", Normal, NormalArray);
            return false;
        }

        // The material is rewritten every time, since the first terrain is what brings each array into being.
        Sequence<MaterialEditor::Binding> Bindings;

        Ref<MaterialEditor::Binding> Colour = Bindings.Append();
        Colour.Name    = Enum::GetName(Texture::Albedo);
        Colour.Path    = StrAfterLast(Splatset::kAlbedo, '/');
        Colour.Inherit = true;

        if (!Normal.IsEmpty() || GetArray(Texture::Normal))
        {
            Ref<MaterialEditor::Binding> Relief = Bindings.Append();
            Relief.Name    = Enum::GetName(Texture::Normal);
            Relief.Path    = StrAfterLast(Splatset::kNormal, '/');
            Relief.Inherit = true;
        }

        MaterialEditor::Write(Resolve(Splatset::kMaterial), Bindings);

        mContext.GetContent().Reload(mSplatset.GetMaterial());

        // The slice the bake appended is the one this terrain draws, and the splatset appends in step.
        const UInt16 Slice = mSplatset.AddTerrain(Name);

        if (Slice == Splatset::kInvalid)
        {
            LOG_E("Splatset: '{0}' is in the arrays, but the palette has no room to name it", Albedo);

            return false;
        }

        mSplatset.GetTerrain(Slice).Relief = !Normal.IsEmpty();
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Assembler::AppendInPlace(Text Albedo, Text Normal, Text Name)
    {
        using namespace Pipeline::Baker::Texture;

        // Slices go into the array the material already holds, which cannot be made bigger once it exists.
        // Without a spare one the caller falls back to assembling the whole thing onto disk again.
        ConstRetainer<Graphic::Image> Colour = GetArray(Texture::Albedo);

        const UInt16 Slice = static_cast<UInt16>(mSplatset.GetTerrains().GetSize());

        if (!Colour || Slice >= Colour->GetLayers())
        {
            return false;
        }

        const Baker Baker(mContext.GetScheduler());

        // Decoding, filtering and transcoding one terrain is all an add costs now, however many the project
        // already holds; what is already in the arrays is never read back.
        const auto Bake = [&](Text Source, Bool Linear, Ref<Bitmap> Output)
        {
            Profile Settings;
            Settings.Linear = Linear;

            const ConstPtr<Importer> Codec = Baker.Find(StrAfterLast(Source, '.'));

            Blob Input;

            if (Codec == nullptr || Filesystem::Read(Source, Input) != Filesystem::Result::Success)
            {
                LOG_E("Splatset: nothing here can read '{0}'", Source);

                return false;
            }

            Surface Decoded = Codec->Import(ConstSpan<Byte>(Input.GetData(), Input.GetSize()), Settings);

            if (!Decoded.IsValid() || Decoded.Slices.IsEmpty())
            {
                LOG_E("Splatset: '{0}' decoded to nothing", Source);

                return false;
            }

            Ref<Bitmap> Frame = Decoded.Slices.GetFront();

            if (Frame.GetWidth() != Colour->GetWidth() || Frame.GetHeight() != Colour->GetHeight())
            {
                LOG_E("Splatset: '{0}' is {1}x{2}, but every slice of the arrays is {3}x{4}",
                    Source, Frame.GetWidth(), Frame.GetHeight(), Colour->GetWidth(), Colour->GetHeight());

                return false;
            }

            Output = Transcoder::Transcode(
                Mipmapper::Generate(Move(Frame), Colour->GetLevels()), Colour->GetFormat());
            return true;
        };

        Bitmap Tone;
        Bitmap Relief;

        if (!Bake(Albedo, false, Tone) || (!Normal.IsEmpty() && !Bake(Normal, true, Relief)))
        {
            return false;
        }

        WriteSlice(Colour, Slice, Tone.GetPixels());

        if (ConstRetainer<Graphic::Image> Array = GetArray(Texture::Normal))
        {
            // The slice is sampled either way, so a terrain that brought no relief is given flat rather
            // than left wearing whatever the array happened to hold there.
            if (Normal.IsEmpty())
            {
                WriteFlatRelief(Array, Slice);
            }
            else
            {
                WriteSlice(Array, Slice, Relief.GetPixels());
            }
        }

        const UInt16 Named = mSplatset.AddTerrain(Name);

        if (Named == Splatset::kInvalid)
        {
            LOG_E("Splatset: '{0}' is in the arrays, but the palette has no room to name it", Albedo);

            return false;
        }

        // The slice is on the GPU now, but the file behind it still ends where it did, so what it owes is
        // remembered and paid on the next save rather than rewritten for every terrain that arrives.
        Ref<Pending> Owed = mPending.Append();
        Owed.Albedo = Albedo;
        Owed.Normal = Normal;

        mSplatset.GetTerrain(Named).Relief = !Normal.IsEmpty();
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Assembler::AppendRelief(UInt16 Slice, Text Source)
    {
        using namespace Pipeline::Baker::Texture;

        const Baker Baker(mContext.GetScheduler());

        Profile Settings;
        Settings.Linear = true;

        const Str                Path  = Resolve(Source);
        const ConstPtr<Importer> Codec = Baker.Find(StrAfterLast(Path, '.'));

        Blob Input;

        if (Codec == nullptr || Filesystem::Read(Path, Input) != Filesystem::Result::Success)
        {
            LOG_E("Splatset: nothing here can read '{0}'", Path);

            return false;
        }

        Surface Decoded = Codec->Import(Input, Settings);

        if (!Decoded.IsValid() || Decoded.Slices.IsEmpty())
        {
            LOG_E("Splatset: '{0}' decoded to nothing", Path);

            return false;
        }

        Ref<Bitmap> Frame = Decoded.Slices.GetFront();

        // The slice is already there holding this terrain's colour, so the relief is written into it rather
        // than the array being remade. A project whose terrains never carried relief has no array to write
        // into at all, and one cannot be conjured here: it has to be baked and bound by the material, which
        // is a save, not a draw.
        ConstRetainer<Graphic::Image> Array = GetArray(Texture::Normal);

        if (!Array)
        {
            LOG_E("Splatset: this project has no normal array; author a terrain with one to bring it about");

            return false;
        }

        if (Frame.GetWidth() != Array->GetWidth() || Frame.GetHeight() != Array->GetHeight())
        {
            LOG_E("Splatset: '{0}' is {1}x{2}, but every slice of the arrays is {3}x{4}",
                Path, Frame.GetWidth(), Frame.GetHeight(), Array->GetWidth(), Array->GetHeight());

            return false;
        }

        const Bitmap Relief = Transcoder::Transcode(
            Mipmapper::Generate(Move(Frame), Array->GetLevels()), Array->GetFormat());

        WriteSlice(Array, Slice, Relief.GetPixels());

        mSplatset.GetTerrain(Slice).Relief = true;
        mReliefs.Assign(Slice, Path);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assembler::Commit()
    {
        using namespace Pipeline::Baker::Texture;

        if (mPending.IsEmpty())
        {
            return;
        }

        const Baker  Baker(mContext.GetScheduler());
        const UInt16 Held = static_cast<UInt16>(mSplatset.GetTerrains().GetSize() - mPending.GetSize());

        // One assemble covers every terrain added since the last save, rather than one per terrain, so what
        // authoring costs no longer depends on how often the project happens to be written out.
        ConstRetainer<Graphic::Image> Shape = GetArray(Texture::Albedo);

        if (!Shape)
        {
            return;
        }

        const auto Flush = [&](Text Array, Bool Linear, Bool Relief)
        {
            Profile Settings;
            Settings.Linear   = Linear;
            Settings.Mipmaps  = true;
            Settings.Compress = true;

            Sequence<Manifest::Entry> Entries;

            for (UInt16 Slice = 0; Slice < Held; ++Slice)
            {
                Ref<Manifest::Entry> Entry = Entries.Append();
                Entry.Source = Array;
                Entry.Slice  = Slice;
                Entry.Width  = Shape->GetWidth();
                Entry.Height = Shape->GetHeight();
            }

            for (ConstRef<Pending> Owed : mPending)
            {
                ConstRef<Str> Source = (Relief ? Owed.Normal : Owed.Albedo);

                // A terrain that brought no relief still needs a slice of the right size, and its colour
                // serves: nothing samples it, because that terrain is drawn flat.
                Ref<Manifest::Entry> Entry = Entries.Append();
                Entry.Source = (Source.IsEmpty() ? Owed.Albedo : Source);
                Entry.Slice  = 0;
                Entry.Width  = Shape->GetWidth();
                Entry.Height = Shape->GetHeight();
            }

            // Relief given to a terrain authored earlier replaces the slice it already holds.
            if (Relief)
            {
                for (UInt32 Index = 0; Index < Entries.GetSize(); ++Index)
                {
                    if (const ConstPtr<Str> Authored = mReliefs.Find(static_cast<UInt16>(Index)))
                    {
                        Entries[Index].Source = (* Authored);
                        Entries[Index].Slice  = 0;
                    }
                }
            }

            // The array is rounded up so the terrains authored before the next save have somewhere to go
            // without it being assembled again.
            for (UInt32 Spare = Entries.GetSize() % kHeadroom; Spare != 0 && Spare < kHeadroom; ++Spare)
            {
                Entries.Append(Entries.GetBack());
            }

            Blob Baked = Baker.Assemble(Entries, Settings);
            return Baked && Filesystem::Write(Array, Baked) == Filesystem::Result::Success;
        };

        if (!Flush(Resolve(Splatset::kAlbedo), false, false))
        {
            LOG_E("Splatset: failed to write '{0}' back", Splatset::kAlbedo);

            return;
        }

        if (GetArray(Texture::Normal) && !Flush(Resolve(Splatset::kNormal), true, true))
        {
            LOG_E("Splatset: failed to write '{0}' back", Splatset::kNormal);

            return;
        }

        mPending.Clear();
        mReliefs.Clear();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assembler::WriteFlatRelief(ConstRetainer<Graphic::Image> Array, UInt16 Slice)
    {
        using namespace Pipeline::Baker::Texture;

        const UInt16 Width  = Array->GetWidth();
        const UInt16 Height = Array->GetHeight();

        // A tangent-space normal pointing straight out of the surface, which is what "no relief" means.
        Blob Pixels = Blob::Allocate<Byte>(static_cast<UInt32>(Width) * Height * 4);

        for (Ptr<UInt8> Texel = Pixels.GetData<UInt8>(), Limit = Texel + Width * Height * 4; Texel < Limit; Texel += 4)
        {
            Texel[0] = 128;
            Texel[1] = 128;
            Texel[2] = 255;
            Texel[3] = 255;
        }

        Bitmap Flat(Graphic::TextureFormat::RGBA8UIntNorm, Width, Height, 1, Move(Pixels));

        WriteSlice(Array, Slice, Transcoder::Transcode(
            Mipmapper::Generate(Move(Flat), Array->GetLevels()), Array->GetFormat()).GetPixels());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assembler::WriteSlice(ConstRetainer<Graphic::Image> Array, UInt16 Slice, ConstSpan<Byte> Pixels)
    {
        Ref<Graphic::Service> Service = mContext.GetGraphic();

        const Graphic::TextureFormat Format = Array->GetFormat();
        const UInt16                 Width  = Array->GetWidth();
        const UInt16                 Height = Array->GetHeight();

        UInt32 Offset = 0;

        for (UInt8 Level = 0; Level < Array->GetLevels(); ++Level)
        {
            const UInt32 Size = Graphic::GetLevelSize(Format, Width, Height, Level);

            // A chain filtered to fewer levels than the array holds simply stops, leaving the rest as they
            // were, rather than reading past the end of what was baked.
            if (Offset + Size > Pixels.GetSize())
            {
                break;
            }

            Blob Data = Blob::Allocate<Byte>(Size);
            Blit(Data.GetData<Byte>(), Size, Pixels.GetData() + Offset);

            Service.UpdateTexture(
                Array->GetHandle(),
                Level,
                Slice,
                0,
                0,
                Graphic::GetLevelExtent(Width,  Level),
                Graphic::GetLevelExtent(Height, Level),
                Graphic::GetLevelPitch(Format, Width, Level),
                Move(Data));

            Offset += Size;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Assembler::Resolve(Text Path)
    {
        static constexpr Text kMount = "Resources://";

        return StrStartsWith(Path, "Resources://")
            ? Str::Print<"{0}/{1}">(mContext.GetProject().GetFolder(), Path.Slice(kMount.GetSize()))
            : Str(Path);
    }
}