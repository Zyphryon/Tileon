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
        SaveDatabase();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::Tick(Real64 Time)
    {
        // The baked arrays arrive asynchronously like any other resource, so they are updated on the first tick.
        if (mDirty && !mAtlases.IsEmpty())
        {
            Update();
        }

        mRegistry.ForEach([this, Time](ConstRef<Motif> Motif)
        {
            Ref<Glyph> Glyph = mGlyphs[Motif.GetID()];

            // The frames sit in consecutive slices, so playing the animation is a step along the array.
            if (Glyph.Count > 0)
            {
                const UInt8 Keyframe = Animator::Sample(Motif.GetAnimation(), Time, 0, Motif.GetEasing());

                Glyph.Slice = Glyph.Start + Min<UInt16>(Keyframe, Glyph.Count - 1);
            }
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::Preload()
    {
        mRegistry.ForEach([this](ConstRef<Motif> Motif)
        {
            Refresh(Motif);
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::Refresh(ConstRef<Motif> Motif)
    {
        Ref<Glyph> Glyph = mGlyphs[Motif.GetID()];

        Glyph.Period  = Motif.GetPeriod();
        Glyph.Tint    = Motif.GetTint();
        Glyph.Texture = 0;
        Glyph.Start   = 0;
        Glyph.Slice   = 0;
        Glyph.Count   = 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::Clone(UInt16 Source, UInt16 Target)
    {
        ConstRef<Motif> SourceMotif = GetMotif(Source);
        Ref<Motif>      TargetMotif = GetMotif(Target);

        TargetMotif.SetMaterial(Content::Uri(SourceMotif.GetMaterial()));
        TargetMotif.SetPeriod(SourceMotif.GetPeriod());
        TargetMotif.SetTint(SourceMotif.GetTint());
        TargetMotif.SetEasing(SourceMotif.GetEasing());
        TargetMotif.SetAnimation(Animation(SourceMotif.GetAnimation()));

        Refresh(TargetMotif);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::Rebind(AnyRef<Sequence<Placement>> Placements)
    {
        mPlacements = Move(Placements);

        for (Ref<Glyph> Glyph : mGlyphs)
        {
            Glyph.Texture = 0;
            Glyph.Start   = 0;
            Glyph.Slice   = 0;
            Glyph.Count   = 0;
        }

        Request();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::Request()
    {
        Ref<Content::Service> Content = GetService<Content::Service>();

        UInt16 Count = 0;

        for (ConstRef<Placement> Placement : mPlacements)
        {
            Count = Max<UInt16>(Count, Placement.Atlas + 1);
        }

        mDirty = true;
        mAtlases.Resize(Count);

        for (UInt16 Index = 0; Index < Count; ++Index)
        {
            if (Ref<Retainer<Graphic::Image>> Atlas = mAtlases[Index]; Atlas)
            {
                Content.Reload(Atlas);
            }
            else
            {
                Atlas = Content.Load<Graphic::Image>(Str::Print<kAtlas>(Index));
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::Update()
    {
        for (ConstRetainer<Graphic::Image> Atlas : mAtlases)
        {
            if (!Atlas || !Atlas->HasFinished())
            {
                return;
            }
        }

        for (ConstRef<Placement> Placement : mPlacements)
        {
            Ref<Glyph> Glyph = mGlyphs[Placement.Motif];

            Glyph.Texture = mAtlases[Placement.Atlas]->GetHandle();
            Glyph.Start   = Placement.Base;
            Glyph.Slice   = Placement.Base;
            Glyph.Count   = Placement.Frames;
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
            Archive.Serialize(mRegistry);
            Archive.Serialize(mPlacements);

            Preload();
            Request();
        }
        else
        {
            LOG_W("Failed to load tileset from '{0}'", Text(kFilename));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Tileset::SaveDatabase()
    {
        Writer Output;

        Archive Archive(Output);
        Archive.Serialize(mRegistry);
        Archive.Serialize(mPlacements);

        GetService<Content::Service>().Write(Text(kFilename), Output.Detach(), { });
    }
}