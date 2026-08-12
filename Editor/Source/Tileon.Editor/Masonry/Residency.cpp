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

#include "Residency.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Masonry
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Residency::Residency(Ref<Engine::Subsystem::Host> Host, Ref<Tileset> Tileset, Ref<Gallery> Gallery)
        : Locator  { Host },
          mTileset { Tileset },
          mGallery { Gallery }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Residency::~Residency()
    {
        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();

        for (ConstRef<Atlas> Atlas : mAtlases)
        {
            if (Atlas.Texture != 0)
            {
                Graphics.DeleteTexture(Atlas.Texture);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Residency::Tick()
    {
        // A bake on its way in binds every glyph it covers, so nothing is copied while it is still arriving.
        if (mTileset.IsLoading())
        {
            return;
        }

        mTileset.ForEachMotif([this](ConstRef<Motif> Motif)
        {
            Ref<Tileset::Glyph> Glyph = mTileset.GetGlyph(Motif.GetID());

            // A motif that let go of its frames gives back the run it was holding before taking another.
            if (Glyph.Texture == 0)
            {
                Demote(Motif.GetID());
                Promote(Motif, Glyph);
            }
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Residency::Promote(ConstRef<Motif> Motif, Ref<Tileset::Glyph> Glyph)
    {
        const Gallery::Cut Cut = mGallery.Measure(Motif);

        if (!Cut.IsValid())
        {
            return;
        }

        // A frame cut out of a sheet cannot carry more mips than its own extent supports.
        UInt8 Levels = Cut.Image->GetLevels();

        while (Levels > 1 && ((Cut.Width >> (Levels - 1)) == 0 || (Cut.Height >> (Levels - 1)) == 0))
        {
            --Levels;
        }

        const UInt16          Index    = Acquire(Cut.Image->GetFormat(), Cut.Width, Cut.Height, Levels, Cut.Frames);
        Ref<Atlas>            Atlas    = mAtlases[Index];
        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();

        // Reuse a run a retired motif gave back when its length matches, otherwise take unused slices.
        UInt16 Base = Atlas.Count;

        const SInt Slot = Atlas.Recycled.Find([Frames = Cut.Frames](ConstRef<Atlas::Run> Run)
        {
            return Run.Count == Frames;
        });

        if (Slot >= 0)
        {
            Base = Atlas.Recycled[Slot].Base;
            Atlas.Recycled.Remove(Slot);
        }
        else
        {
            Atlas.Count += Cut.Frames;
        }

        // The sheet already lives on the GPU, so each frame is blitted out of it rather than re-uploaded.
        ConstRef<Animation> Animation = Motif.GetAnimation();

        for (UInt8 Keyframe = 0; Keyframe < Cut.Frames; ++Keyframe)
        {
            const Rect   Data = Animation.GetFrameData(Keyframe);
            const UInt16 X    = static_cast<UInt16>(Round(Data.GetMinimumX() * Cut.Image->GetWidth()));
            const UInt16 Y    = static_cast<UInt16>(Round(Data.GetMinimumY() * Cut.Image->GetHeight()));

            for (UInt8 Level = 0; Level < Levels; ++Level)
            {
                Graphics.CopyTexture(
                    Cut.Image->GetHandle(),
                    Level,
                    0,
                    X >> Level,
                    Y >> Level,
                    Atlas.Texture,
                    Level,
                    static_cast<UInt16>(Base + Keyframe),
                    0,
                    0,
                    Max<UInt16>(Cut.Width  >> Level, 1),
                    Max<UInt16>(Cut.Height >> Level, 1));
            }
        }

        Glyph.Texture = Atlas.Texture;
        Glyph.Start   = Base;
        Glyph.Slice   = Base;
        Glyph.Count   = Cut.Frames;

        // The run is remembered apart from the glyph, which a bake overwrites without asking.
        mLive[Motif.GetID()] = Tileset::Placement(Motif.GetID(), Base, Cut.Frames, Index);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Residency::Demote(UInt16 ID)
    {
        Ref<Tileset::Placement> Live = mLive[ID];

        if (Live.Frames > 0)
        {
            mAtlases[Live.Atlas].Recycled.Append(Live.Base, Live.Frames);

            Live = Tileset::Placement();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt16 Residency::Acquire(Graphic::TextureFormat Format, UInt16 Width, UInt16 Height, UInt8 Levels, UInt16 Count)
    {
        Ref<Graphic::Service> Graphics = GetService<Graphic::Service>();

        UInt16 Index = static_cast<UInt16>(mAtlases.GetSize());

        for (UInt16 Element = 0; Element < mAtlases.GetSize(); ++Element)
        {
            ConstRef<Atlas> Atlas = mAtlases[Element];

            if (Atlas.Format == Format && Atlas.Width  == Width
             && Atlas.Levels == Levels && Atlas.Height == Height)
            {
                Index = Element;
                break;
            }
        }

        if (Index == mAtlases.GetSize())
        {
            Ref<Atlas> Opened = mAtlases.Append();

            Opened.Format = Format;
            Opened.Width  = Width;
            Opened.Height = Height;
            Opened.Levels = Levels;
        }

        Ref<Atlas> Match = mAtlases[Index];

        // A run the array can already satisfy from what a retired motif gave back needs no room of its own.
        const Bool Reusable = Match.Recycled.Contains([Count](ConstRef<Atlas::Run> Run)
        {
            return Run.Count == Count;
        });

        // Grow when the unused slices cannot hold the run, blitting the old array across so nothing is re-read.
        if (!Reusable && Match.Count + Count > Match.Capacity)
        {
            UInt16 Capacity = Match.Capacity;

            while (Match.Count + Count > Capacity)
            {
                Capacity += kSlices;
            }

            const Graphic::Object Texture = Graphics.CreateTexture(
                Graphic::TextureLayout::Texture2DArray,
                Match.Format,
                Graphic::Storage::Stream,
                Graphic::Usage::Sample,
                Match.Width,
                Match.Height,
                Capacity,
                Match.Levels,
                Graphic::Multisample::X1, Blob());

            if (const Graphic::Object Stale = Match.Texture)
            {
                for (UInt16 Slice = 0; Slice < Match.Count; ++Slice)
                {
                    for (UInt8 Level = 0; Level < Match.Levels; ++Level)
                    {
                        Graphics.CopyTexture(
                            Stale,   Level, Slice, 0, 0,
                            Texture, Level, Slice, 0, 0,
                            Max<UInt16>(Match.Width  >> Level, 1),
                            Max<UInt16>(Match.Height >> Level, 1));
                    }
                }
                Graphics.DeleteTexture(Stale);

                // Every glyph still names the array it was copied into, so they follow it to the new one.
                for (ConstRef<Tileset::Placement> Live : mLive)
                {
                    if (Live.Frames > 0 && Live.Atlas == Index)
                    {
                        mTileset.GetGlyph(Live.Motif).Texture = Texture;
                    }
                }
            }

            Match.Texture  = Texture;
            Match.Capacity = Capacity;
        }
        return Index;
    }
}