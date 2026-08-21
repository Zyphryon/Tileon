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

#include "Palette.hpp"
#include "Tileon.Render/Types.hpp"
#include "Tileon.Editor/Utility.hpp"
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

    static Retainer<Graphic::Image> GetArray(ConstRef<Splatset> Source, Texture Usage)
    {
        ConstRetainer<Graphic::Material> Material = Source.GetMaterial();
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

        const Pipeline::Baker::Texture::Surface Decoded
            = Codec->Import(ConstSpan<Byte>(Input.GetData(), Input.GetSize()), Profile);

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

    Palette::Palette(Ref<Context> Context)
        : Panel       { Context, "Palette", true },
          mRepository { Context.GetRepository() },
          mSplatset   { Context.GetRenderer().GetSplatset() },
          mMode       { -1 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::OnDraw()
    {
        Toolkit::Composer::SetNextWindowSize(300.0f, 500.0f, ImGuiCond_FirstUseEver);
        Toolkit::Composer::SetNextWindowSizeConstraints(200.0f, 300.0f, 800.0f, 1200.0f);

        if (Toolkit::Composer::Begin(GetTitle(), mVisible))
        {
            const Tools::Mode Mode     = GetContext().GetEnum("Tools.Mode", Tools::Mode::Ground);
            const Bool        External = (static_cast<SInt32>(Mode) != mMode);

            if (Toolkit::Composer::BeginTabBar("##palette_tabs"))
            {
                const ImGuiTabItemFlags TerrainFlags =
                    (External && Mode != Tools::Mode::Entity)
                        ? ImGuiTabItemFlags_SetSelected
                        : ImGuiTabItemFlags_None;

                if (Toolkit::Composer::BeginTabItem("Terrain", TerrainFlags))
                {
                    // A tab asked to select itself is only shown as selected on the frame after, so the tab
                    // that is still on screen must not read its own visibility as the user's choice. When the
                    // mode came from somewhere else the tabs follow it; only otherwise do they drive it.
                    if (!External && Mode == Tools::Mode::Entity)
                    {
                        GetContext().SetEnum("Tools.Mode", Tools::Mode::Ground);
                    }

                    DrawTerrainTab();
                    Toolkit::Composer::EndTabItem();
                }

                const ImGuiTabItemFlags EntityFlags =
                    (External && Mode == Tools::Mode::Entity)
                        ? ImGuiTabItemFlags_SetSelected
                        : ImGuiTabItemFlags_None;

                if (Toolkit::Composer::BeginTabItem("Entity", EntityFlags))
                {
                    if (!External && Mode != Tools::Mode::Entity)
                    {
                        GetContext().SetEnum("Tools.Mode", Tools::Mode::Entity);
                    }

                    DrawEntityTab();
                    Toolkit::Composer::EndTabItem();
                }

                Toolkit::Composer::EndTabBar();
            }

            mMode = static_cast<SInt32>(GetContext().GetEnum("Tools.Mode", Tools::Mode::Ground));
        }
        Toolkit::Composer::End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainTab()
    {
        DrawTerrainAuthor();

        mTerrains.DrawToolbar();
        Toolkit::Composer::Separator();

        const Real32 BodyHeight = -(Toolkit::Composer::GetFrameHeightWithSpacing() + 8.0f);
        Toolkit::Composer::BeginChild("##terrain_body", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_None);
        DrawTerrainGallery();
        Toolkit::Composer::EndChild();

        DrawTerrainProperties();

        DrawBottomBar("##terrain_status", [&](Real32)
        {
            DrawTerrainStatus();
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainAuthor()
    {
        Ref<Toolkit::Browser> Browser = GetContext().GetBrowser();

        static constexpr UInt64 kAlbedoKey = "Palette.Terrain.Albedo"_Hash;
        static constexpr UInt64 kNormalKey = "Palette.Terrain.Normal"_Hash;

        if (Str Selection; Browser.Consume(kAlbedoKey, Selection))
        {
            mPendingAlbedo = Move(Selection);
        }
        if (Str Selection; Browser.Consume(kNormalKey, Selection))
        {
            mPendingNormal = Move(Selection);
        }

        const auto DrawSource = [&](Text Label, UInt64 Key, Ref<Str> Value, Text Hint)
        {
            Toolkit::Composer::FieldInline(Label);
            Toolkit::Composer::PushID(Label);
            Toolkit::Composer::InputTextWithButton("##art", Value,
                [&](Text Path)
                {
                    Value = Path;
                },
                ICON_FA_ELLIPSIS,
                [&]
                {
                    Browser.Open(Key, ".png .jpg .tga .bmp .tex");
                },
                ImGuiInputTextFlags_EnterReturnsTrue);
            Toolkit::Composer::PopID();
            Toolkit::Composer::Tooltip(Hint);
        };

        DrawSource("Colour", kAlbedoKey, mPendingAlbedo,
            "The art the terrain is coloured by, which becomes one slice of the tileset."_Text);
        DrawSource("Relief", kNormalKey, mPendingNormal,
            "Its normal map, taken at the same slice, which a project may go without."_Text);

        Toolkit::Composer::FieldInline("Name"_Text);
        Toolkit::Composer::InputText("##name", mPendingName, [&](Text Value)
        {
            mPendingName = Value;
        });
        Toolkit::Composer::Tooltip("What to call the terrain, which nothing but the palette reads."_Text);

        // The arrays are indexed by the same slice, so one may not grow without the other.
        const Bool Missing = GetArray(mSplatset, Texture::Normal) && mPendingNormal.IsEmpty();

        Toolkit::Composer::BeginDisabled(mPendingAlbedo.IsEmpty() || Missing);

        if (Toolkit::Composer::Button(ICON_FA_PLUS " Add Terrain"))
        {
            if (AppendTerrain(mPendingAlbedo, mPendingNormal, mPendingName))
            {
                mPendingAlbedo.Clear();
                mPendingNormal.Clear();
                mPendingName.Clear();
            }
        }
        Toolkit::Composer::EndDisabled();

        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::Button(ICON_FA_MINUS " Remove Terrain"))
        {
            mSplatset.RemoveTerrain(static_cast<UInt16>(GetContext().GetInteger("Selection.Terrain", 0)));
        }
        Toolkit::Composer::Tooltip(
            "Takes the terrain out of the palette. Its art stays in the array, since every painted region "
            "names its art by slice, so whatever was painted with it goes on drawing."_Text);

        if (Missing)
        {
            Toolkit::Composer::Tooltip(
                "This tileset carries relief, so a terrain added without a normal map would read "
                "somebody else's."_Text);
        }
        Toolkit::Composer::Separator();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Palette::AppendTerrain(Text Albedo, Text Normal, Text Name)
    {
        using namespace Pipeline::Baker::Texture;

        const Str AlbedoArray = Resolve(Splatset::kAlbedo);
        const Str NormalArray = Resolve(Splatset::kNormal);

        // A project that has never had a tileset has nowhere to write one yet.
        Filesystem::MakeAll(StrBeforeLast(AlbedoArray, '/'));

        const Baker Baker(GetContext().GetScheduler());

        // Arrays that already exist are grown one slice at a time and written in place, so only the art
        // being added is decoded and filtered. The very first terrain has no array to grow, and no extent
        // or format to match, so that one is still assembled onto disk to bring both into being.
        if (const Str Relief = (Normal.IsEmpty() ? Str() : Resolve(Normal));
            AppendSlice(Resolve(Albedo), Relief, Name))
        {
            return true;
        }

        // One array grows per pass: the colour always, the relief only when the terrain brought one.
        const auto Grow = [&](Text Source, Text Array, Text Mount, ConstRetainer<Graphic::Image> Existing, Bool Linear)
        {
            // Only an array that really loaded has slices worth keeping. One naming a file that is not
            // there has nothing to lose, so its mount is taken over instead, which is the only way out of a
            // stale binding from inside the editor.
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

            // The ground samples an array, and only assembling writes one; baking a lone image would write a
            // flat texture the shader cannot read. So even the first terrain is assembled, from one frame.
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

            return Baked && Filesystem::Write(Array, ConstSpan<Byte>(Baked.GetData(), Baked.GetSize()))
                         == Filesystem::Result::Success;
        };

        if (!Grow(Resolve(Albedo), AlbedoArray, Splatset::kAlbedo, GetArray(mSplatset, Texture::Albedo), false))
        {
            LOG_E("Splatset: failed to add '{0}' to '{1}'", Albedo, AlbedoArray);
            return false;
        }

        if (!Normal.IsEmpty()
            && !Grow(Resolve(Normal), NormalArray, Splatset::kNormal, GetArray(mSplatset, Texture::Normal), true))
        {
            LOG_E("Splatset: failed to add '{0}' to '{1}'", Normal, NormalArray);
            return false;
        }

        // The material is rewritten every time, since the first terrain is what brings each array into being.
        Sequence<MaterialEditor::Binding> Bindings;

        // A path with no schema is joined onto the material's own directory, and the material sits beside
        // its arrays, so each is named by its filename alone.
        // The ground tiles its terrains and filters between them, which the technique already declares, so
        // the material names the art and says nothing about how it is read.
        Ref<MaterialEditor::Binding> Colour = Bindings.Append();
        Colour.Name    = Enum::GetName(Texture::Albedo);
        Colour.Path    = StrAfterLast(Splatset::kAlbedo, '/');
        Colour.Inherit = true;

        if (!Normal.IsEmpty() || GetArray(mSplatset, Texture::Normal))
        {
            Ref<MaterialEditor::Binding> Relief = Bindings.Append();
            Relief.Name    = Enum::GetName(Texture::Normal);
            Relief.Path    = StrAfterLast(Splatset::kNormal, '/');
            Relief.Inherit = true;
        }

        MaterialEditor::Write(Resolve(Splatset::kMaterial), Bindings);

        GetContext().GetContent().Reload(mSplatset.GetMaterial());

        // The slice the bake appended is the one this terrain draws, and the splatset appends in step.
        mSplatset.GetTerrain(mSplatset.AddTerrain(Name)).Relief = !Normal.IsEmpty();
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Palette::AppendSlice(Text Albedo, Text Normal, Text Name)
    {
        using namespace Pipeline::Baker::Texture;

        // Slices go into the array the material already holds, which cannot be made bigger once it exists.
        // Without a spare one the caller falls back to assembling the whole thing onto disk again.
        ConstRetainer<Graphic::Image> Colour = GetArray(mSplatset, Texture::Albedo);

        const UInt16 Slice = CountTerrains();

        if (!Colour || Slice >= Colour->GetLayers())
        {
            return false;
        }

        const Baker Baker(GetContext().GetScheduler());

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

        if (ConstRetainer<Graphic::Image> Array = GetArray(mSplatset, Texture::Normal))
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

        // The slice is on the GPU now, but the file behind it still ends where it did, so what it owes is
        // remembered and paid on the next save rather than rewritten for every terrain that arrives.
        Ref<Pending> Owed = mPending.Append();
        Owed.Albedo = Albedo;
        Owed.Normal = Normal;

        mSplatset.GetTerrain(mSplatset.AddTerrain(Name)).Relief = !Normal.IsEmpty();
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt16 Palette::CountTerrains()
    {
        UInt16 Count = 0;

        mSplatset.ForEachTerrain([&](UInt16, ConstRef<Splatset::Terrain>)
        {
            ++Count;
        });
        return Count;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::WriteFlatRelief(ConstRetainer<Graphic::Image> Array, UInt16 Slice)
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

    void Palette::WriteSlice(ConstRetainer<Graphic::Image> Array, UInt16 Slice, ConstSpan<Byte> Pixels)
    {
        Ref<Graphic::Service> Service = GetContext().GetGraphic();

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

    Str Palette::Resolve(Text Path)
    {
        static constexpr Text kMount = "Resources://";

        // The baker works on disk, so a mounted path has to be resolved before it is handed over: the
        // arrays it writes and the art it reads alike, since the browser hands its selection back as a mount.
        return StrStartsWith(Path, kMount)
            ? Str::Print<"{0}/{1}">(GetContext().GetProject().GetFolder(), Path.Slice(kMount.GetSize()))
            : Str(Path);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Palette::AppendRelief(UInt16 Slice, Text Source)
    {
        using namespace Pipeline::Baker::Texture;

        const Baker Baker(GetContext().GetScheduler());

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

        Surface Decoded = Codec->Import(ConstSpan<Byte>(Input.GetData(), Input.GetSize()), Settings);

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
        ConstRetainer<Graphic::Image> Array = GetArray(mSplatset, Texture::Normal);

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

        Bitmap Relief = Transcoder::Transcode(
            Mipmapper::Generate(Move(Frame), Array->GetLevels()), Array->GetFormat());

        WriteSlice(Array, Slice, Relief.GetPixels());

        mSplatset.GetTerrain(Slice).Relief = true;
        mReliefs.Assign(Slice, Path);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::OnCommit()
    {
        using namespace Pipeline::Baker::Texture;

        if (mPending.IsEmpty())
        {
            return;
        }

        const Baker  Baker(GetContext().GetScheduler());
        const UInt16 Held = CountTerrains() - static_cast<UInt16>(mPending.GetSize());

        // One assemble covers every terrain added since the last save, rather than one per terrain, so what
        // authoring costs no longer depends on how often the project happens to be written out.
        ConstRetainer<Graphic::Image> Shape = GetArray(mSplatset, Texture::Albedo);

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

            return Baked && Filesystem::Write(Array, ConstSpan<Byte>(Baked.GetData(), Baked.GetSize()))
                         == Filesystem::Result::Success;
        };

        if (!Flush(Resolve(Splatset::kAlbedo), false, false))
        {
            LOG_E("Splatset: failed to write '{0}' back", Splatset::kAlbedo);

            return;
        }

        if (GetArray(mSplatset, Texture::Normal) && !Flush(Resolve(Splatset::kNormal), true, true))
        {
            LOG_E("Splatset: failed to write '{0}' back", Splatset::kNormal);

            return;
        }

        mPending.Clear();
        mReliefs.Clear();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainProperties()
    {
        const UInt16 Selection = static_cast<UInt16>(GetContext().GetInteger("Selection.Terrain", 0));

        if (!GetArray(mSplatset, Texture::Albedo) || Selection >= CountTerrains())
        {
            return;
        }

        Ref<Splatset::Terrain> Terrain = mSplatset.GetTerrain(Selection);

        Toolkit::Composer::Section("Terrain");

        // What the terrain is called, which only the palette reads, so it may be changed at any time.
        Toolkit::Composer::FieldInline("Name");
        Toolkit::Composer::PushID("Name");
        Toolkit::Composer::InputText("##value", Terrain.Name, [&](Text Value)
        {
            Terrain.Name = Value;
        });
        Toolkit::Composer::PopID();

        // How often the art repeats over the ground its own size covers, so one slice can be laid down
        // coarse or fine without the art being baked again.
        Toolkit::Composer::FieldInline("Tiling");
        Toolkit::Composer::PushID("Tiling");

        if (Real32 Tiling = Terrain.Tiling; Toolkit::Composer::InputFloat("##value", Tiling, 0.05f, 0.25f, "%.3f"))
        {
            Terrain.Tiling = Clamp(Tiling, 0.01f, 64.0f);
        }
        Toolkit::Composer::PopID();

        // The colour the art is multiplied by, so one slice can dress more than one terrain.
        Toolkit::Composer::FieldInline("Tint");
        Toolkit::Composer::PushID("Tint");
        Toolkit::Composer::InputTintSmall("##value", Terrain.Tint);
        Toolkit::Composer::PopID();

        // Relief may be given long after the terrain was authored, because the slice it goes into is the
        // one its colour already occupies.
        static constexpr UInt64 kReliefKey = "Palette.Terrain.Relief"_Hash;

        Ref<Toolkit::Browser> Browser = GetContext().GetBrowser();

        if (Str Picked; Browser.Consume(kReliefKey, Picked))
        {
            AppendRelief(Selection, Picked);
        }

        // Relief goes into the slice the colour already holds, so it needs an array to be there first.
        const Bool Mounted = (GetArray(mSplatset, Texture::Normal) != nullptr);

        Toolkit::Composer::FieldInline("Relief");
        Toolkit::Composer::PushID("Relief");
        Toolkit::Composer::BeginDisabled(!Mounted);

        if (Toolkit::Composer::Button(Terrain.Relief
                ? ICON_FA_CHECK " Authored"_Text
                : ICON_FA_ELLIPSIS " Set normal map"_Text))
        {
            Browser.Open(kReliefKey, ".png .jpg .tga .bmp .tex");
        }
        Toolkit::Composer::EndDisabled();
        Toolkit::Composer::PopID();

        Toolkit::Composer::Tooltip(!Mounted
            ? "This project has no normal array. Author a terrain with one to bring it about."_Text
            : Terrain.Relief
                ? "The relief this terrain is lit by. Choose again to replace it."_Text
                : "This terrain is lit flat. Choose the normal map it should wear."_Text);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainGallery()
    {
        mTerrains.SetSelection(static_cast<UInt32>(GetContext().GetInteger("Selection.Terrain", 0)));

        // A terrain is a slice of the ground's array, so the gallery is the array itself under the names
        // the splatset holds for it.
        ConstRetainer<Graphic::Image> Array = GetArray(mSplatset, Texture::Albedo);

        if (!Array)
        {
            DrawHint("No terrain array loaded");
            return;
        }

        // Every slice shares one identifier, so which of them is drawn rides in the coordinates.
        const ImTextureID Thumbnail = Plugin::ImGuiRenderer::GetLayeredTextureID(Array->GetHandle());

        mTerrains.Begin();

        mSplatset.ForEachTerrain([&](UInt16 Slice, ConstRef<Splatset::Terrain> Terrain)
        {
            const ImVec2 Minimum = Plugin::ImGuiRenderer::GetLayeredTextureUV(Slice, ImVec2(0.0f, 0.0f));
            const ImVec2 Maximum = Plugin::ImGuiRenderer::GetLayeredTextureUV(Slice, ImVec2(1.0f, 1.0f));

            if (mTerrains.DrawItem(
                    Slice,
                    Terrain.Name.IsEmpty() ? Str::Print<"{0}">(Slice) : Str(Terrain.Name),
                    Thumbnail,
                    Rect(Minimum.x, Minimum.y, Maximum.x, Maximum.y),
                    IntColor8::White()))
            {
                GetContext().SetInteger("Selection.Terrain", Slice);
            }
        });
        mTerrains.End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawTerrainStatus()
    {
        const UInt16 Selection = static_cast<UInt16>(GetContext().GetInteger("Selection.Terrain", 0));

        Str    Name;
        UInt16 Count = 0;
        Bool   Found = false;

        mSplatset.ForEachTerrain([&](UInt16 Slice, ConstRef<Splatset::Terrain> Terrain)
        {
            ++Count;

            if (Slice == Selection)
            {
                Name  = Terrain.Name;
                Found = true;
            }
        });

        if (!Found)
        {
            DrawHint(Count == 0 ? "No terrain array loaded"_Text : "No terrain selected"_Text);
            return;
        }

        Toolkit::Composer::SetCursorPosX(Toolkit::Composer::GetStyle().ItemSpacing.x);
        Toolkit::Composer::Label("{0}  {1} of {2}", Name.IsEmpty() ? "(Unnamed)" : Text(Name), Selection, Count);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawEntityTab()
    {
        mEntities.DrawToolbar();
        Toolkit::Composer::Separator();

        const Real32 BodyHeight = -(Toolkit::Composer::GetFrameHeightWithSpacing() + 8.0f);
        Toolkit::Composer::BeginChild("##entity_body", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_None);
        DrawEntityGallery();
        Toolkit::Composer::EndChild();

        DrawBottomBar("##entity_status", [&](Real32)
        {
            DrawEntityStatus();
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawEntityGallery()
    {
        mEntities.SetSelection(GetContext().GetInteger("Selection.Archetype", 0));

        mEntities.Begin();
        mRepository.ForEachArchetype([&](Scene::Entity Archetype)
        {
            // Only root archetypes are placeable.
            if (Archetype.GetParent(Scene::Hierarchy::Fixed).IsValid())
            {
                return;
            }

            const UInt32 ID = static_cast<UInt32>(Archetype.GetID() - Scene::kMinRangeArchetypes);

            Graphic::Object Thumbnail = 0;
            Rect            Crop      = Rect::One();
            IntColor8       Tint      = IntColor8::White();

            if (const ConstPtr<Appearance> Visual = Archetype.TryGet<const Appearance>())
            {
                ConstRetainer<Graphic::Material> Material = Visual->GetMaterial();

                if (Material && Material->HasCompleted())
                {
                    if (ConstRetainer<Graphic::Image> Albedo = Material->GetImage(GetTextureID(Texture::Albedo)))
                    {
                        Thumbnail = Albedo->GetHandle();
                        Crop      = Visual->GetSource();

                        if (const ConstPtr<IntColor8> Color = Archetype.TryGet<const IntColor8>())
                        {
                            Tint = (* Color);
                        }
                    }
                }
            }

            // An archetype that is still loading, failed, or has no sprite falls back to the gallery's placeholder.
            if (mEntities.DrawItem(ID, Archetype.GetAlias(), Thumbnail, Crop, Tint))
            {
                GetContext().SetInteger("Selection.Archetype", ID);
            }
        });
        mEntities.End();

        // Right-clicking an entity jumps to the Archetypes editor with it selected, keeping palette and editor in sync.
        if (const SInt64 Target = mEntities.GetActivated(); Target >= 0)
        {
            GetContext().SetString("Navigate.Panel", "Archetypes");
            GetContext().SetInteger("Selection.Archetype", Target);
            GetContext().SetInteger("Selection.Archetype.Target", Scene::kMinRangeArchetypes + Target);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawEntityStatus()
    {
        const UInt32        Selection = GetContext().GetInteger("Selection.Archetype", 0);
        const Scene::Entity Archetype = mRepository.GetArchetype(Scene::kMinRangeArchetypes + Selection);

        if (Selection != 0 && Archetype.IsValid())
        {
            const Text Alias = Archetype.GetAlias();

            Toolkit::Composer::SetCursorPosX(Toolkit::Composer::GetStyle().ItemSpacing.x);
            Toolkit::Composer::Label("{0:04}  {1}", Selection, Alias.IsEmpty() ? "(Unnamed)" : Alias);
        }
        else
        {
            DrawHint("No archetype selected");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawHint(Text Hint)
    {
        Toolkit::Composer::SetCursorPosX(
            (Toolkit::Composer::GetWindowWidth() - Toolkit::Composer::CalcTextSize(Hint).x) * 0.5f);
        Toolkit::Composer::TextDisabled(Hint);
    }
}