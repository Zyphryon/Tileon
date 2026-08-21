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

#include "AssetInspector.hpp"
#include "Tileon.Editor/Toolkit/Composer.hpp"
#include "Tileon.Editor/Utility.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool IsReady(ConstRetainer<Content::Resource> Asset)
    {
        if (!Asset || !Asset->HasCompleted())
        {
            Toolkit::Composer::TextDisabled(Asset ? Text("Loading") : Text("Failed to load"));
            return false;
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Inspect(Ref<Toolkit::Previewer> Preview, ConstRetainer<Graphic::Image> Asset)
    {
        if (!IsReady(Asset))
        {
            return;
        }

        const UInt16 Layers = Asset->GetLayers();

        // An array is shown one slice at a time, and the slice rides in the coordinates rather than the
        // handle, so the whole array stays a single texture however many slices it holds.
        Rect Source = Rect::One();

        if (Layers > 1)
        {
            const Real32 Band = Min<UInt16>(Preview.GetSlice(), Layers - 1) * Plugin::ImGuiRenderer::kSliceStride;

            Source.Set(Band, 0.0f, Band + 1.0f, 1.0f);
        }

        const ImTextureID Handle = (Layers > 1
            ? Plugin::ImGuiRenderer::GetLayeredTextureID(Asset->GetHandle())
            : Plugin::ImGuiRenderer::GetTextureID(Asset->GetHandle()));

        // The art leads, in a square of the pane's own width, so how much of it can be seen is not decided
        // by whatever room the fields below happen to leave.
        Toolkit::Composer::BeginChild("##art", ImVec2(0.0f, Toolkit::Composer::GetContentRegionAvail().x));
        Preview.Draw(Handle, Vector2(Asset->GetWidth(), Asset->GetHeight()), Source);
        Toolkit::Composer::EndChild();

        Toolkit::Composer::Separator();

        Toolkit::Composer::FieldInline("Layout");
        Toolkit::Composer::Label(Enum::GetName(Asset->GetLayout()));

        Toolkit::Composer::FieldInline("Format");
        Toolkit::Composer::Label(Enum::GetName(Asset->GetFormat()));

        Toolkit::Composer::FieldInline("Size");
        Toolkit::Composer::Label("{0} x {1}", Asset->GetWidth(), Asset->GetHeight());

        Toolkit::Composer::FieldInline("Mipmaps");
        Toolkit::Composer::Label("{0}", Asset->GetLevels());

        Toolkit::Composer::FieldInline("Slices");
        Toolkit::Composer::Label("{0}", Layers);

        // Stepping the slice reaches the art a frame later, which no eye follows and which keeps the
        // control under the picture it steps rather than above it.
        if (Layers > 1)
        {
            SInt32 Slice = Min<SInt32>(Preview.GetSlice(), Layers - 1);

            Toolkit::Composer::FieldInline("Slice");

            if (Toolkit::Composer::DragInt("##slice", Slice, 0.25f, 0, Layers - 1))
            {
                Preview.Reset();
            }
            Preview.SetSlice(static_cast<UInt16>(Clamp<SInt32>(Slice, 0, Layers - 1)));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Inspect(Ref<Toolkit::Previewer> Preview, ConstRetainer<Graphic::Material> Asset)
    {
        if (!IsReady(Asset))
        {
            return;
        }

        // The loader hashes every name it reads, so a material's own bindings are unreadable at runtime.
        // What can still be told apart are the ones the editor itself has a name for, and that is enough
        // to answer what the panel is really being asked: what art does this material carry.
        Retainer<Graphic::Image> Albedo;

        for (const Texture Usage : Enum::GetValues<Texture>())
        {
            const Retainer<Graphic::Image> Bound = Asset->GetImage(GetTextureID(Usage));

            Toolkit::Composer::FieldInline(Enum::GetName(Usage));

            if (!Bound)
            {
                Toolkit::Composer::TextDisabled("(none)");
                continue;
            }

            if (!Bound->HasCompleted())
            {
                Toolkit::Composer::TextDisabled("Loading");
                continue;
            }

            if (Bound->GetLayers() > 1)
            {
                Toolkit::Composer::Label("{0} x {1}, {2} slices",
                    Bound->GetWidth(), Bound->GetHeight(), Bound->GetLayers());
            }
            else
            {
                Toolkit::Composer::Label("{0} x {1}", Bound->GetWidth(), Bound->GetHeight());
            }

            if (!Albedo)
            {
                Albedo = Bound;
            }
        }

        if (Albedo)
        {
            Toolkit::Composer::Separator();

            Editor::Inspect(Preview, Albedo);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Thumbnail(ConstRetainer<Graphic::Image> Asset, Real32 Size, UInt32 Slice)
    {
        if (!IsReady(Asset))
        {
            return 0;
        }

        const UInt16 Layers = Asset->GetLayers();
        const UInt16 Index  = Min(Slice, Layers - 1);
        const UInt16 Width  = Asset->GetWidth();
        const UInt16 Height = Asset->GetHeight();

        // What it is beats what it is called, which the row under the cursor is already saying.
        Toolkit::Composer::TextDisabled(Layers > 1
            ? String<96>::Print<"{0} / {1}x{2}x{3}">(Enum::GetName(Asset->GetFormat()), Width, Height, Layers)
            : String<96>::Print<"{0} / {1}x{2}">(Enum::GetName(Asset->GetFormat()), Width, Height));

        // The slice rides in the coordinates rather than the handle, so the array stays a single texture.
        const Real32 Band  = (Layers > 1 ? Index * Plugin::ImGuiRenderer::kSliceStride : 0.0f);
        const Real32 Scale = Size / Max(Width, Height);

        Toolkit::Composer::Image(
            Layers > 1 ? Plugin::ImGuiRenderer::GetLayeredTextureID(Asset->GetHandle())
                       : Plugin::ImGuiRenderer::GetTextureID(Asset->GetHandle()),
            ImVec2(Width * Scale, Height * Scale), ImVec4(Band, 0.0f, Band + 1.0f, 1.0f));

        if (Layers > 1)
        {
            Toolkit::Composer::TextDisabled(String<64>::Print<"Slice {0} of {1}">(Index + 1, Layers));
        }
        return Layers;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt32 Thumbnail(ConstRetainer<Graphic::Material> Asset, Real32 Size, UInt32 Binding)
    {
        if (!IsReady(Asset))
        {
            return 0;
        }

        // A material has no face of its own, so it wears one it binds; only the ones it actually carries
        // are gathered, or stepping through them would stop on names that show nothing.
        Array<Texture, Enum::Count<Texture>()> Bound;

        UInt32 Count = 0;

        for (const Texture Usage : Enum::GetValues<Texture>())
        {
            if (Asset->GetImage(GetTextureID(Usage)))
            {
                Bound[Count++] = Usage;
            }
        }

        if (Count == 0)
        {
            return 0;
        }

        const UInt32       Index = Min(Binding, Count - 1);
        const Texture Usage = Bound[Index];

        Toolkit::Composer::Label(Count > 1
            ? String<64>::Print<"{0} ({1} of {2})">(Enum::GetName(Usage), Index + 1, Count)
            : String<64>::Print<"{0}">(Enum::GetName(Usage)));

        // The texture below says its own format and size, so the material only has to name the slot.
        Thumbnail(Asset->GetImage(GetTextureID(Usage)), Size, 0);

        return Count;
    }
}