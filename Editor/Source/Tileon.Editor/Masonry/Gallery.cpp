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

#include "Gallery.hpp"
#include "Tileon.Editor/Component/Texture.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Masonry
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Gallery::Gallery(Ref<Engine::Subsystem::Host> Host)
        : Locator { Host }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Gallery::Cut Gallery::Measure(ConstRef<Motif> Motif)
    {
        Cut Result;

        // A motif that names no frames names no pixels either, and a sheet still loading has no extent.
        Result.Image = Resolve(Motif);

        ConstRef<Animation> Animation = Motif.GetAnimation();
        Result.Frames = Animation.GetCount();

        if (!Result.Image || Result.Frames == 0)
        {
            return Cut();
        }

        // Every frame takes a slice of its own, all of them the extent the first frame declares.
        const Rect First = Animation.GetFrameData(0);
        Result.Width  = static_cast<UInt16>(Round(First.GetWidth()  * Result.Image->GetWidth()));
        Result.Height = static_cast<UInt16>(Round(First.GetHeight() * Result.Image->GetHeight()));

        return Result;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Gallery::Settle(ConstRef<Motif> Motif)
    {
        ConstRef<Retainer<Graphic::Material>> Sheet = Request(Motif);

        // A motif that names no art has nothing to wait for.
        if (!Sheet)
        {
            return true;
        }

        if (!Sheet->HasFinished())
        {
            return false;
        }

        // A sheet settles before the images it names, so the albedo the frames are cut from is awaited too.
        ConstRetainer<Graphic::Image> Image = Sheet->GetImage(GetTextureID(Texture::Albedo));
        return (!Image || Image->HasFinished());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ref<Retainer<Graphic::Material>> Gallery::Request(ConstRef<Motif> Motif)
    {
        Ref<Retainer<Graphic::Material>> Sheet = mSheets[Motif.GetID()];

        // A motif that names no art gives back the sheet it was holding, which nothing else keeps alive.
        ConstRef<Content::Uri> Url = Motif.GetMaterial();

        if (!Url.IsValid())
        {
            Sheet = Retainer<Graphic::Material>();

            return Sheet;
        }

        // Loading is keyed by uri, so a motif pointed at art another one already named costs nothing.
        if (!Sheet || Sheet->GetKey().Hash() != Url.Hash())
        {
            Sheet = GetService<Content::Service>().Load<Graphic::Material>(Url);
        }
        return Sheet;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Retainer<Graphic::Image> Gallery::Resolve(ConstRef<Motif> Motif)
    {
        Ref<Retainer<Graphic::Material>> Sheet = Request(Motif);

        if (!Sheet || !Sheet->HasFinished())
        {
            return Retainer<Graphic::Image>();
        }

        Retainer<Graphic::Image> Image = Sheet->GetImage(GetTextureID(Texture::Albedo));

        if (!Image || !Image->HasFinished() || Image->GetHandle() == 0)
        {
            return Retainer<Graphic::Image>();
        }
        return Image;
    }
}