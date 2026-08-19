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

#include "Tileon.Editor/Asset/Editor/TextureEditor.hpp"
#include "Tileon.Editor/Toolkit/Composer.hpp"
#include <Baker.Texture/Baker.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    TextureEditor::TextureEditor(Ref<Context> Context)
        : mContext   { Context },
          mExtentX   { 0 },
          mExtentY   { 0 },
          mSelection { -1 },
          mOpen      { false },
          mDirty     { false },
          mClosing   { false }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool TextureEditor::Create(Text Path, AnyRef<Content::Uri> Key)
    {
        mPath      = Path;
        mKey       = Move(Key);
        mImage     = nullptr;
        mExtentX   = 0;
        mExtentY   = 0;
        mSelection = -1;
        mOpen      = true;
        mDirty     = true;

        mSlots.Clear();

        // Nothing reaches disk until the first slice gives the array an extent to bake against, so the
        // folder in view has nothing new to show yet.
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void TextureEditor::Open(Text Path, AnyRef<Content::Uri> Key)
    {
        mPath      = Path;
        mKey       = Move(Key);
        mImage     = mContext.GetContent().Load<Graphic::Image>(mKey);
        mSelection = -1;
        mOpen      = true;
        mDirty     = false;

        mSlots.Clear();

        // The array itself is the authority on how many slices exist; the catalogue only names them, and a
        // catalogue that has fallen behind is filled in rather than believed.
        const UInt16 Layers = (mImage && mImage->HasCompleted() ? mImage->GetLayers() : 0);

        mExtentX = (mImage && mImage->HasCompleted() ? mImage->GetWidth()  : 0);
        mExtentY = (mImage && mImage->HasCompleted() ? mImage->GetHeight() : 0);

        ReadCatalogue(Layers);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sequence<TextureEditor::Slot> TextureEditor::Read(Text Path)
    {
        Sequence<Slot> Slots;

        Blob File;

        if (Filesystem::Read(Str::Print<"{0}{1}">(Path, kCatalogue), File) != Filesystem::Result::Success || File == nullptr)
        {
            return Slots;
        }

        if (JsonValue Document = JsonDocument::Parse(Text(File.GetData<Char>(), File.GetSize())); Document.IsObject())
        {
            const JsonArray Slices = JsonObject(Document).GetArray("Slices");

            for (UInt Index = 0; !Slices.IsNullOrEmpty() && Index < Slices.GetSize(); ++Index)
            {
                const JsonObject Entry = Slices.GetObject(Index);

                Ref<Slot> Record = Slots.Append();
                Record.Name    = Entry.GetString("Name");
                Record.Retired = Entry.GetBool("Retired", false);
            }
        }
        return Slots;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str TextureEditor::Locate(Text Url) const
    {
        // The browser answers with the url an asset is loaded under, but the baker reads it off disk.
        return Str::Print<"{0}/{1}">(mContext.GetProject().GetFolder(), Content::Uri(Str(Url)).GetPath());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void TextureEditor::Adopt(Text Url)
    {
        const Str  Path = Locate(Url);
        const Text Stem = StrBeforeLast(StrAfterLast(Url, '/'), '.');

        UInt16 Slices = 1;
        UInt16 Width  = 0;
        UInt16 Height = 0;

        // A baked array brings all of its slices over rather than just the first, which is what makes one
        // array buildable out of another. Anything else contributes the one frame it is.
        if (StrEndsWith(Url, ".tex"))
        {
            Measure(Path, Width, Height, Slices);
        }

        // An array with no slices yet has no extent either, so the first source brings one with it.
        if (mExtentX == 0 && Width > 0)
        {
            mExtentX = Width;
            mExtentY = Height;
        }

        for (UInt16 Index = 0; Index < Slices; ++Index)
        {
            Ref<Slot> Record = mSlots.Append();
            Record.Source = Path;
            Record.Slice  = Index;
            Record.Name   = (Slices > 1 ? Str::Print<"{0} {1:03}">(Stem, Index) : Str(Stem));
        }

        mSelection = static_cast<SInt32>(mSlots.GetSize()) - 1;
        mDirty     = true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void TextureEditor::ReadCatalogue(UInt16 Layers)
    {
        mSlots = Read(mPath);

        // A slice the catalogue never mentioned is still a slice, so it is listed unnamed rather than lost.
        while (mSlots.GetSize() < Layers)
        {
            mSlots.Append();
        }

        // The other way round means the catalogue outlived the array it described, so the surplus is dropped.
        while (mSlots.GetSize() > Layers)
        {
            mSlots.RemoveLast();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool TextureEditor::WriteCatalogue() const
    {
        JsonValue Document;
        Document.SetObject();

        JsonArray Slices = JsonObject(Document).SetArray("Slices");

        for (ConstRef<Slot> Record : mSlots)
        {
            JsonObject Entry = Slices.AddObject();
            Entry.SetString("Name", Record.Name);
            Entry.SetBool("Retired", Record.Retired);
        }

        const Str Data = JsonDocument::Dump(Document);

        if (Filesystem::Write(Str::Print<"{0}{1}">(mPath, kCatalogue), Data) != Filesystem::Result::Success)
        {
            LOG_E("TextureEditor: failed to write the catalogue for '{0}'", mPath);
            return false;
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Text TextureEditor::Reason() const
    {
        if (mSlots.IsEmpty())
        {
            return "Add a slice before saving";
        }

        if (mExtentX == 0 || mExtentY == 0)
        {
            return "Give the slices a size before saving";
        }
        return Text();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void TextureEditor::DrawClosing()
    {
        if (!mClosing)
        {
            return;
        }

        // Nothing is at stake, so leaving needs no answer.
        if (!mDirty)
        {
            mClosing = false;
            mOpen    = false;
            return;
        }

        Toolkit::Composer::OpenPopup("Discard Slices");

        if (!Toolkit::Composer::BeginPopupModal("Discard Slices"))
        {
            return;
        }

        Toolkit::Composer::Label("The slices added since the last bake have not been written.");
        Toolkit::Composer::Separator();

        if (Toolkit::Composer::Button("Discard", 96.0f))
        {
            mClosing = false;
            mOpen    = false;

            Toolkit::Composer::CloseCurrentPopup();
        }

        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::Button("Cancel", 96.0f))
        {
            mClosing = false;

            Toolkit::Composer::CloseCurrentPopup();
        }

        Toolkit::Composer::EndPopup();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool TextureEditor::Measure(Text Path, Ref<UInt16> Width, Ref<UInt16> Height, Ref<UInt16> Slices)
    {
        constexpr UInt kHeader = 14;

        Filesystem::Handle Handle;

        if (Filesystem::Open(Path, Filesystem::Access::Read, Handle) != Filesystem::Result::Success)
        {
            LOG_W("TextureEditor: failed to open '{0}'", Path);
            return false;
        }

        Array<Byte, kHeader> Bytes { };

        const Filesystem::Result Result = Filesystem::Read(Handle, 0, Span(Bytes.GetData(), kHeader));
        Filesystem::Close(Handle);

        if (Result != Filesystem::Result::Success)
        {
            LOG_W("TextureEditor: failed to read the header of '{0}'", Path);
            return false;
        }

        // Without this a file that merely ends in .tex would have four arbitrary bytes taken as its extent.
        if (Bytes[0] != 'Z' || Bytes[1] != 'T' || Bytes[2] != 'E' || Bytes[3] != 'X')
        {
            LOG_W("TextureEditor: '{0}' is not a baked texture", Path);
            return false;
        }

        Width  = static_cast<UInt16>(Bytes[8]  | (Bytes[9]  << 8));
        Height = static_cast<UInt16>(Bytes[10] | (Bytes[11] << 8));
        Slices = Max<UInt16>(static_cast<UInt16>(Bytes[12] | (Bytes[13] << 8)), 1);

        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool TextureEditor::Save()
    {
        if (mSlots.IsEmpty() || mExtentX == 0 || mExtentY == 0)
        {
            LOG_E("TextureEditor: '{0}' names no slices to bake", mPath);
            return false;
        }

        Sequence<Pipeline::Baker::Texture::Manifest::Entry> Entries;

        for (UInt32 Index = 0; Index < mSlots.GetSize(); ++Index)
        {
            ConstRef<Slot> Record = mSlots[Index];

            Ref<Pipeline::Baker::Texture::Manifest::Entry> Entry = Entries.Append();
            Entry.Width  = mExtentX;
            Entry.Height = mExtentY;

            if (Record.Source.IsEmpty())
            {
                // Nothing new was chosen for this slot, so the slice it already holds is carried straight
                // through. That is what lets a retired slice keep its art and every region keep drawing.
                Entry.Source = mPath;
                Entry.Slice  = static_cast<UInt16>(Index);
            }
            else
            {
                Entry.Source = Record.Source;
                Entry.Slice  = Record.Slice;
            }
        }

        Pipeline::Baker::Texture::Profile Profile;
        Profile.Compress = true;

        // The array keeps whatever it was baked as, so replacing one slice never quietly restates the rest.
        if (mImage && mImage->HasCompleted())
        {
            Profile.Format  = mImage->GetFormat();
            Profile.Mipmaps = (mImage->GetLevels() > 1);
        }
        else
        {
            Profile.Mipmaps = true;
            Profile.Linear  = false;
        }

        const Pipeline::Baker::Texture::Baker Baker(mContext.GetScheduler());

        Blob Baked = Baker.Assemble(Entries, Profile);

        if (Baked == nullptr)
        {
            LOG_E("TextureEditor: failed to assemble '{0}'", mPath);
            return false;
        }

        if (Filesystem::Write(mPath, ConstSpan<Byte>(Baked.GetData<Byte>(), Baked.GetSize())) != Filesystem::Result::Success)
        {
            LOG_E("TextureEditor: failed to write '{0}'", mPath);
            return false;
        }

        WriteCatalogue();

        // Everything holding the old array has to be told, and a reload reaches the materials that bind it.
        Ref<Content::Service> Content = mContext.GetContent();
        Content.Reload(Content.Load<Graphic::Image>(mKey));

        // What was chosen has been baked in, so the slots go back to carrying their own slices through.
        for (Ref<Slot> Record : mSlots)
        {
            Record.Source.Clear();
        }

        mImage = Content.Load<Graphic::Image>(mKey);
        mDirty = false;
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool TextureEditor::Draw()
    {
        if (!mOpen)
        {
            return false;
        }

        Bool Written = false;

        Toolkit::Composer::SetNextWindowSize(720.0f, 460.0f, ImGuiCond_FirstUseEver);

        if (Toolkit::Composer::Begin("Texture", mOpen))
        {
            Toolkit::Composer::Label(mPath);

            if (mExtentX > 0)
            {
                Toolkit::Composer::SameLine();
                Toolkit::Composer::TextDisabled(
                    String<64>::Print<"  {0} x {1}, {2} slice(s)">(mExtentX, mExtentY, mSlots.GetSize()));
            }
            else
            {
                // Every slice of an array shares one extent, and art on disk does not announce its own
                // until it is decoded. An array started from a baked one inherits it; otherwise it is said
                // here, once, and every slice is cut to it.
                Toolkit::Composer::FieldInline("Slice size");
                Toolkit::Composer::InputIntPair<UInt16>("##extent", mExtentX, mExtentY);
            }

            Toolkit::Composer::Separator();

            const Real32 Width = Toolkit::Composer::GetContentRegionAvail().x * 0.55f;

            Toolkit::Composer::BeginChild("##slots", ImVec2(Width, -Toolkit::Composer::GetFrameHeightWithSpacing()));
            DrawSlots();
            Toolkit::Composer::EndChild();

            Toolkit::Composer::SameLine();

            Toolkit::Composer::BeginChild("##slice", ImVec2(0.0f, -Toolkit::Composer::GetFrameHeightWithSpacing()));
            DrawPreview();
            Toolkit::Composer::EndChild();

            // Saying why the bake cannot run beats a Save button that quietly does nothing.
            const Text Blocked = Reason();

            if (Toolkit::Composer::DisabledButton("Save", !mDirty || !Blocked.IsEmpty(), 96.0f))
            {
                Written = Save();
            }

            Toolkit::Composer::SameLine();

            if (Toolkit::Composer::Button("Close", 96.0f))
            {
                mClosing = true;
            }

            if (!Blocked.IsEmpty())
            {
                Toolkit::Composer::SameLine();
                Toolkit::Composer::TextDisabled(Blocked);
            }
            else if (mDirty)
            {
                Toolkit::Composer::SameLine();
                Toolkit::Composer::TextDisabled("Unsaved changes");
            }

            DrawClosing();
        }
        Toolkit::Composer::End();

        // The window's own close button clears the flag straight through, so leaving that way is caught
        // here rather than slipping past the prompt the Close button raises.
        if (!mOpen && mDirty)
        {
            mOpen    = true;
            mClosing = true;
        }
        return Written;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void TextureEditor::DrawSlots()
    {
        constexpr UInt64 kAppend = "TextureEditor.Append"_Hash;

        if (Toolkit::Composer::Button(ICON_FA_PLUS "  Add slice"))
        {
            mContext.GetBrowser().Open(kAppend, kSources);
        }

        // A slot is what everything else stores, so a new slice is only ever appended: the ones already
        // there keep the numbers the regions painted with.
        if (Str Selection; mContext.GetBrowser().Consume(kAppend, Selection))
        {
            Adopt(Selection);
        }

        Toolkit::Composer::Separator();

        for (UInt32 Index = 0; Index < mSlots.GetSize(); ++Index)
        {
            Ref<Slot> Record = mSlots[Index];

            Toolkit::Composer::PushID(String<32>::Print<"slot_{0}">(Index));

            if (Toolkit::Composer::Selectable(
                    String<160>::Print<"{0:03}  {1}{2}">(Index,
                        Record.Name.IsEmpty() ? "(unnamed)" : Record.Name,
                        Record.Retired ? "  (retired)" : (Record.Source.IsEmpty() ? "" : "  (pending)")),
                    mSelection == static_cast<SInt32>(Index)))
            {
                mSelection = static_cast<SInt32>(Index);
                mPreview.Reset();
            }

            if (Toolkit::Composer::BeginPopupContextItem())
            {
                if (Toolkit::Composer::MenuItem(ICON_FA_PEN "  Replace..."))
                {
                    mContext.GetBrowser().Open("TextureEditor.Replace"_Hash + Index, kSources);
                }

                // Never "remove": a slot is an identifier, and taking one out would renumber every slice
                // after it and silently repoint everything that had painted with them.
                if (Toolkit::Composer::MenuItem(Record.Retired
                        ? Text(ICON_FA_ROTATE_LEFT "  Restore")
                        : Text(ICON_FA_BAN "  Retire")))
                {
                    Record.Retired = !Record.Retired;
                    mDirty         = true;
                }
                Toolkit::Composer::EndPopup();
            }

            // Replacing takes only the first slice of whatever was chosen, because the slot it lands in
            // is an identity that a second slice would have nowhere to go.
            if (Str Selection; mContext.GetBrowser().Consume("TextureEditor.Replace"_Hash + Index, Selection))
            {
                Record.Source = Locate(Selection);
                Record.Slice  = 0;
                mDirty        = true;
            }

            Toolkit::Composer::PopID();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void TextureEditor::DrawPreview()
    {
        if (mSelection < 0 || mSelection >= static_cast<SInt32>(mSlots.GetSize()))
        {
            Toolkit::Composer::TextDisabled("No slice selected");
            return;
        }

        Ref<Slot> Record = mSlots[mSelection];

        Toolkit::Composer::FieldInline("Name");

        Toolkit::Composer::InputText("##name", Record.Name, [&](Text Value)
        {
            Record.Name = Value;
            mDirty      = true;
        });

        if (!Record.Source.IsEmpty())
        {
            Toolkit::Composer::TextDisabled(Record.Source);
            Toolkit::Composer::TextDisabled("Baked in on save");
            return;
        }

        if (!mImage || !mImage->HasCompleted() || mSelection >= mImage->GetLayers())
        {
            Toolkit::Composer::TextDisabled("Not baked yet");
            return;
        }

        const Real32 Band = mSelection * Plugin::ImGuiRenderer::kSliceStride;

        mPreview.Draw(Plugin::ImGuiRenderer::GetLayeredTextureID(mImage->GetHandle()),
            Vector2(mExtentX, mExtentY), Rect(Band, 0.0f, Band + 1.0f, 1.0f));
    }
}