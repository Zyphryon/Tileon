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
#include <Baker.Texture/Exporter.hpp>
#include <Baker.Texture/Process/Compositor.hpp>
#include <Baker.Texture/Process/Mipmapper.hpp>
#include <Baker.Texture/Process/Resampler.hpp>
#include <Baker.Texture/Process/Transcoder.hpp>
#include <Zyphryon.Graphic/Metadata.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool Decode(
        ConstRef<Pipeline::Baker::Texture::Baker> Baker,
        Text                                      Source,
        Ref<Pipeline::Baker::Texture::Bitmap>     Output,
        Bool                                      Linear,
        UInt16                                    Slice = 0)
    {
        using namespace Pipeline::Baker::Texture;

        Profile Settings;
        Settings.Linear = Linear;

        const ConstPtr<Importer> Codec = Baker.Find(StrAfterLast(Source, '.'));

        Blob Input;

        if (Codec == nullptr || Filesystem::Read(Source, Input) != Filesystem::Result::Success)
        {
            LOG_E("Splatset: nothing here can read '{0}'", Source);

            return false;
        }

        Surface Decoded = Codec->Import(Input, Settings);

        if (!Decoded.IsValid() || Decoded.Slices.IsEmpty())
        {
            LOG_E("Splatset: '{0}' decoded to nothing", Source);

            return false;
        }

        if (Slice >= Decoded.Slices.GetSize())
        {
            LOG_E("Splatset: '{0}' holds no slice {1}", Source, Slice);

            return false;
        }

        Output = Move(Decoded.Slices[Slice]);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Pipeline::Baker::Texture::Bitmap Blank(UInt16 Width, UInt16 Height, ConstRef<Array<UInt8, 4>> Colour)
    {
        Blob Pixels = Blob::Allocate<Byte>(static_cast<UInt32>(Width) * Height * 4);

        for (Ptr<UInt8> Texel = Pixels.GetData<UInt8>(),
             Limit = Texel + static_cast<UInt32>(Width) * Height * 4; Texel < Limit; Texel += 4)
        {
            Texel[0] = Colour[0];
            Texel[1] = Colour[1];
            Texel[2] = Colour[2];
            Texel[3] = Colour[3];
        }

        return Pipeline::Baker::Texture::Bitmap(
            Graphic::TextureFormat::RGBA8UIntNorm, Width, Height, 1, Move(Pixels));
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

    Assembler::Assembler(Ref<Context> Context, Ref<Splatset> Splatset)
        : mContext  { Context },
          mSplatset { Splatset },
          mDirty    { false }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt16 Assembler::Create()
    {
        const UInt16 Slice = mSplatset.AddTerrain(Text());

        if (Slice == Splatset::kInvalid)
        {
            return Splatset::kInvalid;
        }

        // An array baked with room to spare takes the terrain now; one without owes a re-bake at the save.
        mDirty = true;

        if (ConstRetainer<Graphic::Image> Colour = GetArray(Texture::Albedo); Colour && Slice < Colour->GetLayers())
        {
            WriteArt(Slice, Texture::Albedo);

            if (GetArray(Texture::Normal))
            {
                WriteArt(Slice, Texture::Normal);
            }
        }
        else
        {
            // Without a slice of its own the terrain would show whatever the array happens to hold there,
            // so the arrays are baked again at once rather than left owing until the project is saved.
            if (!Rebuild())
            {
                return Splatset::kInvalid;
            }
        }
        return Slice;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Assembler::SetSource(UInt16 Slice, Slot Slot, Text Source)
    {
        if (Slice >= mSplatset.GetTerrains().GetSize())
        {
            return false;
        }

        Ref<Splatset::Terrain> Terrain = mSplatset.GetTerrain(Slice);

        switch (Slot)
        {
        case Slot::Albedo:
            Terrain.Albedo = Source;
            break;
        case Slot::Normal:
            Terrain.Normal = Source;
            break;
        case Slot::Height:
            Terrain.Height = Source;
            break;
        }

        // Relief brings an array into being the first time one is asked for, and height rides in the alpha
        // of the colour, so only a slice that is already there can be written where it stands.
        const Texture Written = (Slot == Slot::Normal) ? Texture::Normal : Texture::Albedo;

        // The slice on the card is patched where it stands so the change shows at once, but the arrays on
        // disk are stale either way, so the save is owed a bake regardless of which path was taken.
        mDirty = true;

        if (ConstRetainer<Graphic::Image> Array = GetArray(Written); Array && Slice < Array->GetLayers())
        {
            return WriteArt(Slice, Written);
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Assembler::Rebuild()
    {
        using namespace Pipeline::Baker::Texture;

        const ConstSpan<Splatset::Terrain> Terrains = mSplatset.GetTerrains();

        if (Terrains.IsEmpty())
        {
            return true;
        }

        ConstRetainer<Graphic::Image> Colour = GetArray(Texture::Albedo);
        ConstRetainer<Graphic::Image> Relief = GetArray(Texture::Normal);

        // The size the project authors at, which every slice is scaled to fit whatever its art measures.
        const UInt16 Extent = mSplatset.GetResolution();

        const Baker Baker(mContext.GetScheduler());
        const Str   AlbedoArray = Resolve(Splatset::kAlbedo);
        const Str   NormalArray = Resolve(Splatset::kNormal);

        Bool Lit    = (Relief != nullptr);
        Bool Raised = false;

        for (ConstRef<Splatset::Terrain> Terrain : Terrains)
        {
            Lit    |= !Terrain.Normal.IsEmpty();
            Raised |= !Terrain.Height.IsEmpty();
        }

        Sequence<Bitmap> Tones;
        Sequence<Bitmap> Reliefs;

        for (UInt16 Slice = 0; Slice < Terrains.GetSize(); ++Slice)
        {
            ConstRef<Splatset::Terrain> Terrain = Terrains[Slice];

            // A terrain that named its art is baked from it again; one seeded from an array nobody authored
            // through is read back out of that array, so a rebuild never costs it the slice it holds.
            Bitmap Tone;

            if (!Terrain.Albedo.IsEmpty())
            {
                if (!Decode(Baker, Resolve(Terrain.Albedo), Tone, false))
                {
                    return false;
                }

                if (!Terrain.Height.IsEmpty())
                {
                    Bitmap Raised;

                    // The relief a terrain stands at rides in the alpha of its colour, which nothing else claims.
                    if (Decode(Baker, Resolve(Terrain.Height), Raised, true))
                    {
                        if (Bitmap Merged = Compositor::Insert(Tone, Raised, 3, 0); !Merged.GetPixels().IsEmpty())
                        {
                            Tone = Move(Merged);
                        }
                    }
                }
            }
            else if (!Colour || !Decode(Baker, AlbedoArray, Tone, false, Slice))
            {
                Tone = Blank(Extent, Extent, Array<UInt8, 4> { 128, 128, 128, 128 });
            }

            Tones.Append(Resampler::Resize(Tone, Extent, Extent));

            if (!Lit)
            {
                continue;
            }

            Bitmap Surface;

            if (!Terrain.Normal.IsEmpty())
            {
                if (!Decode(Baker, Resolve(Terrain.Normal), Surface, true))
                {
                    return false;
                }
            }
            else if (!Relief || !Decode(Baker, NormalArray, Surface, true, Slice))
            {
                Surface = Blank(Extent, Extent, Array<UInt8, 4> { 128, 128, 255, 255 });
            }

            Reliefs.Append(Resampler::Resize(Surface, Extent, Extent));
        }

        // Room is left over so terrains authored before the next save can be written where they stand.
        while (Tones.GetSize() % kHeadroom != 0)
        {
            Tones.Append(Blank(Extent, Extent, Array<UInt8, 4> { 128, 128, 128, 128 }));

            if (Lit)
            {
                Reliefs.Append(Blank(Extent, Extent, Array<UInt8, 4> { 128, 128, 255, 255 }));
            }
        }

        Profile Settings;
        Settings.Mipmaps  = true;
        Settings.Compress = true;

        Settings.Linear = false;

        if (Blob Baked = Exporter::Export(
            mContext.GetScheduler(), Move(Tones), Graphic::TextureLayout::Texture2DArray, Settings);
            !Baked || Filesystem::Write(AlbedoArray, Baked) != Filesystem::Result::Success)
        {
            LOG_E("Splatset: failed to write '{0}' back", Splatset::kAlbedo);

            return false;
        }

        if (Lit)
        {
            Settings.Linear = true;

            if (Blob Baked = Exporter::Export(
                mContext.GetScheduler(), Move(Reliefs), Graphic::TextureLayout::Texture2DArray, Settings);
                !Baked || Filesystem::Write(NormalArray, Baked) != Filesystem::Result::Success)
            {
                LOG_E("Splatset: failed to write '{0}' back", Splatset::kNormal);

                return false;
            }
        }

        WriteMaterial(Lit, Raised);

        mContext.GetContent().Reload(mSplatset.GetMaterial());
        mDirty = false;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assembler::Commit()
    {
        if (mDirty)
        {
            Rebuild();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Assembler::WriteArt(UInt16 Slice, Texture Usage)
    {
        using namespace Pipeline::Baker::Texture;

        ConstRetainer<Graphic::Image> Store = GetArray(Usage);

        // The array is baked with room to spare, so a slice inside it may still name no terrain at all.
        if (!Store || Slice >= Store->GetLayers() || Slice >= mSplatset.GetTerrains().GetSize())
        {
            return false;
        }

        ConstRef<Splatset::Terrain> Terrain = mSplatset.GetTerrain(Slice);
        ConstRef<Str>               Source  = (Usage == Texture::Normal) ? Terrain.Normal : Terrain.Albedo;

        const Baker  Baker(mContext.GetScheduler());
        const UInt16 Extent = Store->GetWidth();

        Bitmap Frame;

        if (Source.IsEmpty())
        {
            Frame = (Usage == Texture::Normal)
                ? Blank(Extent, Extent, Array<UInt8, 4> { 128, 128, 255, 255 })
                : Blank(Extent, Extent, Array<UInt8, 4> { 128, 128, 128, 128 });
        }
        else
        {
            if (!Decode(Baker, Resolve(Source), Frame, Usage == Texture::Normal))
            {
                return false;
            }

            Frame = Resampler::Resize(Frame, Extent, Store->GetHeight());

            // The relief a terrain stands at rides in the alpha of its colour, which nothing else claims.
            if (Usage == Texture::Albedo && !Terrain.Height.IsEmpty())
            {
                Bitmap Raised;

                if (Decode(Baker, Resolve(Terrain.Height), Raised, true))
                {
                    if (Bitmap Merged = Compositor::Insert(Frame, Raised, 3, 0); !Merged.GetPixels().IsEmpty())
                    {
                        Frame = Move(Merged);
                    }
                }
            }
        }

        WriteSlice(Store, Slice, Transcoder::Transcode(
            Mipmapper::Generate(Move(Frame), Store->GetLevels()), Store->GetFormat()).GetPixels());
        return true;
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

    void Assembler::WriteMaterial(Bool Lit, Bool Raised)
    {
        // A path with no schema is joined onto the material's own directory, and the material sits beside
        // its arrays, so each is named by its filename alone.
        Sequence<MaterialEditor::Binding> Bindings;

        Ref<MaterialEditor::Binding> Colour = Bindings.Append();
        Colour.Name    = Enum::GetName(Texture::Albedo);
        Colour.Path    = StrAfterLast(Splatset::kAlbedo, '/');
        Colour.Inherit = true;

        if (Lit)
        {
            Ref<MaterialEditor::Binding> Relief = Bindings.Append();
            Relief.Name    = Enum::GetName(Texture::Normal);
            Relief.Path    = StrAfterLast(Splatset::kNormal, '/');
            Relief.Inherit = true;
        }

        Sequence<MaterialEditor::Constant> Constants;

        // The technique turns its height blend on wherever the parameter is there at all, so art that
        // carries no height leaves it out of the material rather than writing it false.
        if (Raised)
        {
            Ref<MaterialEditor::Constant> Height = Constants.Append();
            Height.Name    = "Height";
            Height.Type    = Graphic::Uniform::Bool;
            Height.Boolean = true;
        }

        MaterialEditor::Write(Resolve(Splatset::kMaterial), Bindings, Constants);
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