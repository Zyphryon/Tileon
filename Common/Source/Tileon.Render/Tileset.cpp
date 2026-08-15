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

#include "Tileset.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Tileset::Tileset(Ref<Engine::Subsystem::Host> Host)
        : Locator { Host },
          mDirty  { true }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::Load()
    {
        GetService<Content::Service>().Read(Text(kFilename), [this](Filesystem::Result Result, Blob Data)
        {
            LoadDatabase(Result, Move(Data));
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::Save()
    {
        Writer Output;

        Archive Archive(Output);
        Sequence<Content::Uri> Urls;

        for (ConstRef<Atlases> Atlas : mAtlases)
        {
            Urls.Append(Atlas[Enum::Cast(Motif::Source::Albedo)]->GetKey());
        }

        Archive.Serialize(mRegistry);
        Archive.Serialize(Urls);
        Archive.Serialize(mPlacements);

        GetService<Content::Service>().Write(Text(kFilename), Output.Detach(), { });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::Tick(Real64 Time)
    {
        // The baked Atlases arrive asynchronously like any other resource, so binding waits for the tick they land on.
        if (mDirty)
        {
            Update();
        }

        mRegistry.ForEach([this, Time](ConstRef<Motif> Motif)
        {
            Ref<Glyph> Glyph = mGlyphs[Motif.GetID()];

            // The glyph is what a draw reads, so it follows whatever the motif was authored with.
            Glyph.Period = Motif.GetPeriod();
            Glyph.Tint   = Motif.GetTint();

            // The frames sit in consecutive slices, so playing the animation is a step along the array.
            if (Glyph.Count > 0)
            {
                const UInt32 Keyframe = Motif.GetFlipbook().Locate(Time, Motif.GetEasing());

                Glyph.Slice = Glyph.Start + Min<UInt16>(Keyframe, Glyph.Count - 1);
            }
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::Clone(UInt16 Source, UInt16 Target)
    {
        ConstRef<Motif> SourceMotif = GetMotif(Source);
        Ref<Motif>      TargetMotif = GetMotif(Target);

        TargetMotif.SetPeriod(SourceMotif.GetPeriod());
        TargetMotif.SetTint(SourceMotif.GetTint());
        TargetMotif.SetEasing(SourceMotif.GetEasing());

        for (const Motif::Source Slot : Enum::GetValues<Motif::Source>())
        {
            TargetMotif.SetSource(Slot, Content::Uri(SourceMotif.GetSource(Slot)));
        }

        TargetMotif.SetFlipbook(SourceMotif.GetFlipbook());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::Rebind(AnyRef<Sequence<Placement>> Placements)
    {
        mPlacements = Move(Placements);

        for (Ref<Glyph> Glyph : mGlyphs)
        {
            Glyph.Textures = { };
            Glyph.Start    = 0;
            Glyph.Slice    = 0;
            Glyph.Count    = 0;
        }

        Request();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt16 Tileset::GetOrInsertAtlas(AnyRef<Content::Uri> Url)
    {
        for (UInt16 Index = 0; Index < mAtlases.GetSize(); ++Index)
        {
            if (mAtlases[Index][Enum::Cast(Motif::Source::Albedo)]->GetKey().GetUrl() == Url.GetUrl())
            {
                return Index;
            }
        }

        // The atlas holds the url from here on, whether or not anything has been written under it yet.
        Ref<Content::Service> Service = GetService<Content::Service>();

        // A sheet whose motifs name no map of a texture has no array for it, which simply fails to load.
        Ref<Atlases> Atlas = mAtlases.Append();

        for (const Motif::Source Slot : Enum::GetValues<Motif::Source>())
        {
            Atlas[Enum::Cast(Slot)] = Service.Load<Graphic::Image>(GetAtlasUrl(Url.GetUrl(), Slot));
        }
        return static_cast<UInt16>(mAtlases.GetSize() - 1);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::Request()
    {
        Ref<Content::Service> Content = GetService<Content::Service>();

        mDirty = true;

        // An array still arriving refuses a reload, so it is dropped outright and asked for again.
        for (Ref<Atlases> Atlas : mAtlases)
        {
            for (Ref<Retainer<Graphic::Image>> Array : Atlas)
            {
                const Content::Uri Url = Array->GetKey();

                Content.Unload(Array);

                Array = Content.Load<Graphic::Image>(Url);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::Update()
    {
        // An array a sheet was never fired for fails rather than arrives, which is what leaves it unbound.
        for (ConstRef<Atlases> Atlas : mAtlases)
        {
            for (ConstRetainer<Graphic::Image> Array : Atlas)
            {
                if (!Array->HasFinished())
                {
                    return;
                }
            }
        }

        for (ConstRef<Placement> Placement : mPlacements)
        {
            Ref<Glyph>        Glyph = mGlyphs[Placement.Motif];
            ConstRef<Atlases> Atlas = mAtlases[Placement.Atlas];

            for (const Motif::Source Slot : Enum::GetValues<Motif::Source>())
            {
                ConstRetainer<Graphic::Image> Array = Atlas[Enum::Cast(Slot)];

                Glyph.Textures[Enum::Cast(Slot)] = Array->HasFailed() ? 0 : Array->GetHandle();
            }

            Glyph.Start = Placement.Base;
            Glyph.Slice = Placement.Base;
            Glyph.Count = Placement.Frames;
        }

        mDirty = false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::LoadDatabase(Filesystem::Result Result, Blob Data)
    {
        if (Result == Filesystem::Result::Success)
        {
            Reader  Input(Data.GetData(), Data.GetSize());

            Archive Archive(Input);
            Sequence<Content::Uri> Urls;

            Archive.Serialize(mRegistry);
            Archive.Serialize(Urls);
            Archive.Serialize(mPlacements);

            Ref<Content::Service> Service = GetService<Content::Service>();

            for (Ref<Content::Uri> Url : Urls)
            {
                Ref<Atlases> Atlas = mAtlases.Append();

                for (const Motif::Source Slot : Enum::GetValues<Motif::Source>())
                {
                    Atlas[Enum::Cast(Slot)] = Service.Load<Graphic::Image>(GetAtlasUrl(Url.GetUrl(), Slot));
                }
            }

            mDirty = true;
        }
        else
        {
            LOG_W("Failed to load tileset from '{0}'", Text(kFilename));
        }
    }
}