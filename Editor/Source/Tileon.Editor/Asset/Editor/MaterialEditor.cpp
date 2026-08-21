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

#include "Tileon.Editor/Asset/Editor/MaterialEditor.hpp"
#include <Zyphryon.Graphic/Material.hpp>
#include <Zyphryon.Graphic/Metadata.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    MaterialEditor::MaterialEditor(Ref<Context> Context)
        : mContext { Context },
          mDirty   { false },
          mClosing { false }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool MaterialEditor::Create(Text Path, AnyRef<Content::Uri> Key)
    {
        // An empty material is still a valid one, which is what lets the editor open it and fill it in.
        if (!Write(Path, ConstSpan<Binding>()))
        {
            return false;
        }

        // Creating and editing are one motion, so the material opens where it was just written.
        mPath    = Path;
        mKey     = Move(Key);
        mDirty   = false;
        mClosing = false;

        mBindings.Clear();
        mConstants.Clear();

        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void MaterialEditor::Open(Text Path, AnyRef<Content::Uri> Key)
    {
        mPath    = Path;
        mKey     = Move(Key);
        mDirty   = false;
        mClosing = false;

        mBindings.Clear();
        mConstants.Clear();

        Blob File;

        if (Filesystem::Read(mPath, File) != Filesystem::Result::Success)
        {
            LOG_E("MaterialEditor: failed to read '{0}'", mPath);
            return;
        }

        JsonValue Document = JsonDocument::Parse(Text(File.GetData<Char>(), File.GetSize()));

        if (!Document.IsObject())
        {
            LOG_E("MaterialEditor: '{0}' is not a material", mPath);
            return;
        }

        const JsonObject Root = JsonObject(Document);

        // The loader hashes every name it reads, so the file is the only place they still exist as text.
        const JsonObject Images   = Root.GetObject("Images");
        const JsonObject Samplers = Root.GetObject("Samplers");

        if (Images.IsValid())
        {
            for (ConstRef<JsonValue::Object::Pair> Pair : Images.GetNode()->GetObject())
            {
                Binding Entry;
                Entry.Name = Pair.First;
                Entry.Path = Images.GetObject(Pair.First).GetString("Path");

                // A texture is read through the sampler that shares its name, when the material declares one.
                if (const JsonObject Sampler = Samplers.IsValid() ? Samplers.GetObject(Pair.First) : JsonObject(); Sampler.IsValid())
                {
                    Entry.Filter   = Sampler.GetEnum("Filter", Graphic::TextureFilter::Point);
                    Entry.AddressU = Sampler.GetEnum("AddressModeU", Graphic::TextureAddress::Clamp);
                    Entry.AddressV = Sampler.GetEnum("AddressModeV", Graphic::TextureAddress::Clamp);
                    Entry.AddressW = Sampler.GetEnum("AddressModeW", Graphic::TextureAddress::Clamp);
                }

                mBindings.Append(Move(Entry));
            }
        }

        if (const JsonObject Parameters = Root.GetObject("Parameters"); Parameters.IsValid())
        {
            for (ConstRef<JsonValue::Object::Pair> Pair : Parameters.GetNode()->GetObject())
            {
                const JsonObject Source = Parameters.GetObject(Pair.First);

                Constant Entry;
                Entry.Name = Pair.First;
                Entry.Type = Enum::Cast(Source.GetString("Type"), Graphic::Uniform::Float);

                const Graphic::UniformMetadata Metadata = Graphic::GetUniformMetadata(Entry.Type);

                if (Entry.Type == Graphic::Uniform::Bool)
                {
                    Entry.Boolean = Source.GetBool("Value");
                }
                else if (const JsonArray Values = Source.GetArray("Value"); !Values.IsNullOrEmpty())
                {
                    for (UInt32 Index = 0; Index < 4 && Index < Values.GetSize(); ++Index)
                    {
                        if (Metadata.IsInteger())
                        {
                            Entry.Integer[Index] = Values.GetNumber<SInt32>(Index);
                        }
                        else
                        {
                            Entry.Decimal[Index] = Values.GetNumber<Real32>(Index);
                        }
                    }
                }
                else if (Metadata.IsInteger())
                {
                    Entry.Integer[0] = Source.GetNumber<SInt32>("Value", 0);
                }
                else
                {
                    Entry.Decimal[0] = Source.GetNumber<Real32>("Value", 0.0f);
                }

                mConstants.Append(Move(Entry));
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool MaterialEditor::Draw()
    {
        if (mPath.IsEmpty())
        {
            return false;
        }

        Bool Saved   = false;
        Bool Visible = true;

        Toolkit::Composer::SetNextWindowSize(520.0f, 420.0f, ImGuiCond_FirstUseEver);

        if (Toolkit::Composer::Begin("Material##Editor", Visible))
        {
            Toolkit::Composer::TextDisabled(StrAfterLast(mPath, '/'));

            Toolkit::Composer::Section("Textures");

            constexpr Text kHint = "Choose a name";

            SInt32 Discard = -1;

            for (UInt32 Index = 0; Index < mBindings.GetSize(); ++Index)
            {
                Ref<Binding> Entry = mBindings[Index];

                Toolkit::Composer::PushID(Index);

                Toolkit::Composer::FieldInline("Name");

                if (Toolkit::Composer::BeginCombo("##Name", Entry.Name.IsEmpty() ? kHint : Text(Entry.Name)))
                {
                    Bool Known = false;

                    for (const Texture Usage : Enum::GetValues<Texture>())
                    {
                        const Text Name     = Enum::GetName(Usage);
                        const Bool Selected = (Entry.Name == Name);
                        Known = Known || Selected;

                        if (Toolkit::Composer::Selectable(Name, Selected))
                        {
                            Entry.Name = Name;
                            mDirty     = true;
                        }
                    }

                    // A material may bind a name the list no longer covers, and opening it is no reason to
                    // lose that, so whatever it already carries is offered alongside the names that are.
                    if (!Known && !Entry.Name.IsEmpty())
                    {
                        Toolkit::Composer::Selectable(Entry.Name, true);
                    }

                    Toolkit::Composer::EndCombo();
                }

                const UInt64 Key = HashCombine("Assets.Material.Path", Index);

                if (Str Selection; mContext.GetBrowser().Consume(Key, Selection))
                {
                    Entry.Path = Move(Selection);
                    mDirty     = true;
                }

                Toolkit::Composer::FieldInline("Path");
                Toolkit::Composer::InputTextWithButton("##Path", Entry.Path,
                    [this, &Entry](Text Value)
                    {
                        Entry.Path = Value;
                        mDirty     = true;
                    },
                    ICON_FA_ELLIPSIS,
                    [this, Key]
                    {
                        mContext.GetBrowser().Open(Key, ".tex");
                    },
                    ImGuiInputTextFlags_EnterReturnsTrue);

                // A texture and the sampler that reads it are authored together.
                Toolkit::Composer::FieldInline("Filter");
                mDirty |= Toolkit::Composer::Combo("##Filter", Entry.Filter);

                Toolkit::Composer::FieldInline("Address U");
                mDirty |= Toolkit::Composer::Combo("##AddressU", Entry.AddressU);

                Toolkit::Composer::FieldInline("Address V");
                mDirty |= Toolkit::Composer::Combo("##AddressV", Entry.AddressV);

                Toolkit::Composer::FieldInline("Address W");
                mDirty |= Toolkit::Composer::Combo("##AddressW", Entry.AddressW);

                if (Toolkit::Composer::Button(ICON_FA_TRASH "  Remove"))
                {
                    Discard = static_cast<SInt32>(Index);
                }

                Toolkit::Composer::PopID();
                Toolkit::Composer::Separator();
            }

            if (Discard >= 0)
            {
                mBindings.Remove(static_cast<UInt32>(Discard));

                mDirty = true;
            }

            if (Toolkit::Composer::Button(ICON_FA_PLUS "  Add Texture"))
            {
                Ref<Binding> Entry = mBindings.Append();

                for (const Texture Usage : Enum::GetValues<Texture>())
                {
                    const Text Name  = Enum::GetName(Usage);
                    const Bool Taken = mBindings.Contains([&Name](ConstRef<Binding> Other)
                    {
                        return Other.Name == Name;
                    });

                    if (!Taken)
                    {
                        Entry.Name = Name;
                        break;
                    }
                }

                mDirty = true;
            }

            Toolkit::Composer::Section("Parameters");

            Discard = -1;

            if (Toolkit::Composer::BeginTable("##Constants", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV))
            {
                Toolkit::Composer::TableSetupColumn("Name");
                Toolkit::Composer::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 96.0f);
                Toolkit::Composer::TableSetupColumn("Value");

                for (UInt32 Index = 0; Index < mConstants.GetSize(); ++Index)
                {
                    Ref<Constant> Entry = mConstants[Index];

                    Toolkit::Composer::TableNextRow();
                    Toolkit::Composer::TableNextColumn();
                    Toolkit::Composer::PushID(Index);

                    Toolkit::Composer::InputText("##Name", Entry.Name, [this, &Entry](Text Value)
                    {
                        Entry.Name = Value;
                        mDirty     = true;
                    });

                    Toolkit::Composer::TableNextColumn();

                    mDirty |= Toolkit::Composer::Combo("##Type", Entry.Type);

                    Toolkit::Composer::TableNextColumn();

                    if (Entry.Type == Graphic::Uniform::Bool)
                    {
                        mDirty |= Toolkit::Composer::Checkbox("##Value", Entry.Boolean);
                    }
                    else
                    {
                        const Graphic::UniformMetadata Metadata = Graphic::GetUniformMetadata(Entry.Type);

                        const UInt32 Count    = Metadata.Components;
                        const Bool   Integral = Metadata.IsInteger();

                        for (UInt32 Component = 0; Component < Count; ++Component)
                        {
                            if (Component > 0)
                            {
                                Toolkit::Composer::SameLine();
                            }

                            Toolkit::Composer::PushID(Component);
                            Toolkit::Composer::SetNextItemWidth(64.0f);

                            if (Integral)
                            {
                                mDirty |= Toolkit::Composer::DragInt("##Value", Entry.Integer[Component]);
                            }
                            else
                            {
                                mDirty |= Toolkit::Composer::DragFloat("##Value", Entry.Decimal[Component], 0.01f);
                            }
                            Toolkit::Composer::PopID();
                        }
                    }

                    Toolkit::Composer::SameLine();

                    if (Toolkit::Composer::SmallButton(ICON_FA_TRASH))
                    {
                        Discard = static_cast<SInt32>(Index);
                    }

                    Toolkit::Composer::PopID();
                }

                Toolkit::Composer::EndTable();
            }

            if (Discard >= 0)
            {
                mConstants.Remove(static_cast<UInt32>(Discard));

                mDirty = true;
            }

            if (Toolkit::Composer::Button(ICON_FA_PLUS "  Add Parameter"))
            {
                mConstants.Append();

                mDirty = true;
            }

            Toolkit::Composer::Separator();

            const Text Blocked = Reason();

            if (Toolkit::Composer::DisabledButton("Save", !mDirty || !Blocked.IsEmpty(), 96.0f))
            {
                // Only a write that landed counts as saved; a failed one keeps the window and the edits.
                Saved = Save();
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

        if (!Visible)
        {
            mClosing = true;
        }
        return Saved;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool MaterialEditor::Write(Text Path, ConstSpan<Binding> Bindings, ConstSpan<Constant> Constants)
    {
        JsonValue Document;
        Document.SetObject();

        JsonObject Root     = JsonObject(Document);
        JsonObject Images   = Root.SetObject("Images");

        JsonObject Samplers;

        for (ConstRef<Binding> Entry : Bindings)
        {
            if (Entry.Name.IsEmpty())
            {
                LOG_W("MaterialEditor: dropped an unnamed texture bound to '{0}'", Entry.Path);
                continue;
            }

            Images.SetObject(Entry.Name).SetString("Path", Entry.Path);

            // A material overrides a sampler only where it sets one, so a binding that inherits writes
            // none and leaves the technique's own declaration standing.
            if (Entry.Inherit)
            {
                continue;
            }

            if (!Samplers.IsValid())
            {
                Samplers = Root.SetObject("Samplers");
            }

            // Every texture leaves with the sampler that reads it, named the same, as the loader expects.
            JsonObject Sampler = Samplers.SetObject(Entry.Name);
            Sampler.SetEnum("Filter", Entry.Filter);
            Sampler.SetEnum("AddressModeU", Entry.AddressU);
            Sampler.SetEnum("AddressModeV", Entry.AddressV);
            Sampler.SetEnum("AddressModeW", Entry.AddressW);
        }

        if (!Constants.IsEmpty())
        {
            JsonObject Parameters = Root.SetObject("Parameters");

            for (ConstRef<Constant> Entry : Constants)
            {
                if (Entry.Name.IsEmpty())
                {
                    LOG_W("MaterialEditor: dropped an unnamed parameter");
                    continue;
                }

                JsonObject Value = Parameters.SetObject(Entry.Name);
                Value.SetString("Type", Enum::GetName(Entry.Type));

                const Graphic::UniformMetadata Metadata = Graphic::GetUniformMetadata(Entry.Type);

                const UInt32 Count    = Metadata.Components;
                const Bool   Integral = Metadata.IsInteger();

                if (Entry.Type == Graphic::Uniform::Bool)
                {
                    Value.SetBool("Value", Entry.Boolean);
                }
                else if (Count > 1)
                {
                    JsonArray Components = Value.SetArray("Value");

                    for (UInt32 Index = 0; Index < Count; ++Index)
                    {
                        if (Integral)
                        {
                            Components.AddNumber(Entry.Integer[Index]);
                        }
                        else
                        {
                            Components.AddNumber(Entry.Decimal[Index]);
                        }
                    }
                }
                else if (Integral)
                {
                    Value.SetNumber("Value", Entry.Integer[0]);
                }
                else
                {
                    Value.SetNumber("Value", Entry.Decimal[0]);
                }
            }
        }

        const Str Data = JsonDocument::Dump(Document);

        if (Filesystem::Write(Path, Data) != Filesystem::Result::Success)
        {
            LOG_E("MaterialEditor: failed to write '{0}'", Path);
            return false;
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void MaterialEditor::DrawClosing()
    {
        if (!mClosing)
        {
            return;
        }

        // Nothing is at stake, so leaving needs no answer.
        if (!mDirty)
        {
            mClosing = false;
            mPath.Clear();
            return;
        }

        Toolkit::Composer::OpenPopup("Discard Changes");

        if (!Toolkit::Composer::BeginPopupModal("Discard Changes"))
        {
            return;
        }

        Toolkit::Composer::Label("The changes made since the last save have not been written.");
        Toolkit::Composer::Separator();

        if (Toolkit::Composer::Button("Discard", 96.0f))
        {
            mClosing = false;
            mPath.Clear();

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

    Text MaterialEditor::Reason() const
    {
        for (UInt32 Index = 0; Index < mBindings.GetSize(); ++Index)
        {
            ConstRef<Binding> Entry = mBindings[Index];

            if (Entry.Name.IsEmpty())
            {
                return "Name every texture before saving";
            }

            if (Entry.Path.IsEmpty())
            {
                return "Give every texture a file before saving";
            }

            for (UInt32 Other = 0; Other < Index; ++Other)
            {
                if (mBindings[Other].Name == Entry.Name)
                {
                    return "Two textures share a name";
                }
            }
        }

        for (UInt32 Index = 0; Index < mConstants.GetSize(); ++Index)
        {
            ConstRef<Constant> Entry = mConstants[Index];

            if (Entry.Name.IsEmpty())
            {
                return "Name every parameter before saving";
            }

            for (UInt32 Other = 0; Other < Index; ++Other)
            {
                if (mConstants[Other].Name == Entry.Name)
                {
                    return "Two parameters share a name";
                }
            }
        }
        return Text();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool MaterialEditor::Save()
    {
        if (!Write(mPath, mBindings, mConstants))
        {
            return false;
        }

        mDirty = false;

        // Saving is only half of it; the material in memory is still the one that was read.
        Ref<Content::Service> Service = mContext.GetContent();

        Service.Reload(Service.Load<Graphic::Material>(mKey));

        return true;
    }
}