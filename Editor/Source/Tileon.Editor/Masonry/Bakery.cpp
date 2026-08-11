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

#include "Bakery.hpp"
#include <Baker.hpp>
#include <Importer/STBImporter.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Masonry
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Str Locate(Text Folder, Text Sheet)
    {
        const Text Stem = StrBeforeLast(Sheet, '.');

        for (const Text Type : Pipeline::Baker::Image::STBImporter::kTypes)
        {
            Str Candidate = Str::Print<"{0}/{1}.{2}">(Folder, Stem, Type);

            Filesystem::Handle Handle;

            if (Filesystem::Open(Candidate, Filesystem::Access::Read, Handle) == Filesystem::Result::Success)
            {
                Filesystem::Close(Handle);

                return Candidate;
            }
        }
        return Str();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bakery::Bakery(Ref<Engine::Subsystem::Host> Host, Ref<Tileset> Tileset, Ref<Gallery> Gallery)
        : Locator  { Host },
          mTileset { Tileset },
          mGallery { Gallery }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Bakery::Bake(Text Folder)
    {
        Sequence<Group>              Groups;
        Sequence<Tileset::Placement> Placements;

        // A motif left out of the walk would be bound to slices no array holds, so nothing is written at all.
        if (!Gather(Folder, Groups, Placements))
        {
            return false;
        }

        // A motif is only gathered once its sheet has arrived, so an empty walk means none of them had, and
        // writing that out would replace a good bake with one naming nothing.
        if (Placements.IsEmpty())
        {
            LOG_W("Tileset: nothing to bake, no motif has a sheet to cut frames from");

            return false;
        }

        // A half-written bake would place motifs into arrays that never arrived, so it is adopted whole.
        if (!Assemble(Folder, Groups))
        {
            return false;
        }

        // A bake lays the arrays out from scratch, so one left over from a wider bake names slices this one
        // never wrote, and a project that stops naming it would still ship it.
        Discard(Folder, Groups.GetSize());

        LOG_I("Tileset: baked {0} motifs into {1} arrays", Placements.GetSize(), Groups.GetSize());

        mTileset.Rebind(Move(Placements));

        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Bakery::Prepare()
    {
        Bool Ready = true;

        mTileset.GetRegistry().ForEach([this, &Ready](ConstRef<Motif> Motif)
        {
            Ready = mGallery.Settle(Motif) && Ready;
        });
        return Ready;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Bakery::Discard(Text Folder, UInt32 Count)
    {
        // The arrays are numbered without gaps, so the first one missing is past the last a bake ever wrote.
        for (UInt32 Index = Count; ; ++Index)
        {
            const Str          Atlas = Str::Print<kAtlas>(Folder, Index);
            Filesystem::Handle Handle;

            if (Filesystem::Open(Atlas, Filesystem::Access::Read, Handle) != Filesystem::Result::Success)
            {
                break;
            }
            Filesystem::Close(Handle);

            if (Filesystem::Delete(Atlas) == Filesystem::Result::Success)
            {
                LOG_I("Tileset: discarded '{0}', which this bake no longer names", Atlas);
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Bakery::Gather(Text Folder, Ref<Sequence<Group>> Groups, Ref<Sequence<Tileset::Placement>> Placements)
    {
        Bool Gathered = true;

        mTileset.GetRegistry().ForEach([&](ConstRef<Motif> Motif)
        {
            const Gallery::Cut Cut = mGallery.Measure(Motif);

            if (!Cut.IsValid())
            {
                if (Motif.GetMaterial().IsValid() && Motif.GetAnimation().GetCount() > 0)
                {
                    LOG_E("Tileset: motif {0} names '{1}', which never arrived to cut frames from",
                        Motif.GetID(), Motif.GetMaterial().GetUrl());

                    Gathered = false;
                }
                return;
            }

            // A material names the baked sheet, which no importer reads back, so the frames are cut out of
            // the art it was baked from instead.
            const Str Source = Locate(Folder, Cut.Image->GetKey().GetPath());

            if (Source.IsEmpty())
            {
                LOG_E("Tileset: no source art beside '{0}', which motif {1} cuts its frames from",
                    Cut.Image->GetKey().GetPath(), Motif.GetID());

                Gathered = false;
                return;
            }

            // Every slice of an array shares one extent, so a frame of a size no group holds opens a new one.
            SInt Index = Groups.Find([&Cut](ConstRef<Group> Batch)
            {
                return Batch.Width == Cut.Width && Batch.Height == Cut.Height;
            });

            if (Index < 0)
            {
                Index = static_cast<SInt>(Groups.GetSize());

                Groups.Append(Cut.Width, Cut.Height);
            }

            Ref<Group> Batch = Groups[Index];

            Ref<Tileset::Placement> Placement = Placements.Append();
            Placement.Motif  = Motif.GetID();
            Placement.Base   = Batch.Count;
            Placement.Frames = Cut.Frames;
            Placement.Atlas  = static_cast<UInt16>(Index);

            Batch.Count += Cut.Frames;

            // The baker cuts every frame out of the sheet on disk, which the material names and the project roots.
            ConstRef<Animation> Animation = Motif.GetAnimation();

            for (UInt8 Keyframe = 0; Keyframe < Cut.Frames; ++Keyframe)
            {
                const Rect Data = Animation.GetFrameData(Keyframe);

                Ref<Entry> Record = Batch.Entries.Append();

                Record.Source = Source;
                Record.X      = static_cast<UInt16>(Round(Data.GetMinimumX() * Cut.Image->GetWidth()));
                Record.Y      = static_cast<UInt16>(Round(Data.GetMinimumY() * Cut.Image->GetHeight()));
                Record.Width  = Cut.Width;
                Record.Height = Cut.Height;
            }
        });

        return Gathered;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Bakery::Assemble(Text Folder, ConstRef<Sequence<Group>> Groups)
    {
        const Pipeline::Baker::Image::Baker Assembler(GetService<Job::Service>());

        // Tile art is authored in sRGB, so it bakes to an sRGB format and the GPU decodes it on sample.
        Pipeline::Baker::Image::Profile Settings;
        Settings.Linear  = false;
        Settings.Mipmaps = true;

        Bool Written = true;

        for (UInt32 Index = 0; Index < Groups.GetSize(); ++Index)
        {
            ConstRef<Group> Batch = Groups[Index];

            const Str  Atlas  = Str::Print<kAtlas>(Folder, Index);
            const Blob Output = Assembler.Assemble(Batch.Entries, Settings);

            if (Output == nullptr)
            {
                LOG_E("Tileset: failed to assemble '{0}'", Atlas);

                Written = false;
                continue;
            }

            Filesystem::Ensure(Atlas);

            if (Filesystem::Write(Atlas, Output) != Filesystem::Result::Success)
            {
                LOG_E("Tileset: failed to write '{0}'", Atlas);

                Written = false;
            }
        }
        return Written;
    }
}