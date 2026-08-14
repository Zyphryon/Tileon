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

#include "Materializer.hpp"
#include <Zyphryon.Graphic/Material.hpp>
#include <Zyphryon.Graphic/Metadata.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Materializer::Materializer(Ref<Context> Context)
        : mContext { Context },
          mBrowser { Context.GetContent() }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Materializer::Create(Text Path, AnyRef<Content::Uri> Key)
    {
        // An empty material is still a valid one, which is what lets the editor open it and fill it in.
        if (!Write(Path, ConstSpan<Binding>()))
        {
            return false;
        }

        // Creating and editing are one motion, so the material opens where it was just written.
        mPath = Str(Path);
        mKey  = Move(Key);

        mBindings.Clear();
        mConstants.Clear();

        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Materializer::Open(Text Path, AnyRef<Content::Uri> Key)
    {
        mPath = Str(Path);
        mKey  = Move(Key);

        mBindings.Clear();
        mConstants.Clear();

        Blob File;

        if (Filesystem::Read(mPath, File) != Filesystem::Result::Success)
        {
            LOG_E("Materializer: failed to read '{0}'", mPath);
            return;
        }

        JsonValue Document = JsonDocument::Parse(Text(File.GetData<Char>(), File.GetSize()));

        if (!Document.IsObject())
        {
            LOG_E("Materializer: '{0}' is not a material", mPath);
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
                Entry.Name = Str(Pair.First);
                Entry.Path = Str(Images.GetObject(Pair.First).GetString("Path"));

                // A texture is read through the sampler that shares its name, when the material declares one.
                if (const JsonObject Sampler = Samplers.IsValid() ? Samplers.GetObject(Pair.First) : JsonObject();
                    Sampler.IsValid())
                {
                    Entry.Filter  = Sampler.GetEnum("Filter", Graphic::TextureFilter::Point);
                    Entry.Address = Sampler.GetEnum("AddressModeU", Graphic::TextureAddress::Clamp);
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
                Entry.Name = Str(Pair.First);
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

    Bool Materializer::Draw()
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

            // Textures.
            Toolkit::Composer::Section("Textures");

            SInt32 Discard = -1;

            for (UInt32 Index = 0; Index < mBindings.GetSize(); ++Index)
            {
                Ref<Binding> Entry = mBindings[Index];

                Toolkit::Composer::PushID(Index);

                Toolkit::Composer::FieldInline("Name");
                Toolkit::Composer::InputText("##Name", Entry.Name, [&Entry](Text Value)
                {
                    Entry.Name = Str(Value);
                });

                const UInt64 Key = HashCombine("Assets.Material.Path", Index);

                if (Str Selection; mBrowser.Consume(Key, Selection))
                {
                    Entry.Path = Move(Selection);
                }

                Toolkit::Composer::FieldInline("Path");
                Toolkit::Composer::InputTextWithButton("##Path", Entry.Path,
                    [&Entry](Text Value)
                    {
                        Entry.Path = Str(Value);
                    },
                    ICON_FA_ELLIPSIS,
                    [this, Key]
                    {
                        mBrowser.Open(Key, ".tex");
                    },
                    ImGuiInputTextFlags_EnterReturnsTrue);

                // A texture and the sampler that reads it are authored together, since one is useless alone.
                Toolkit::Composer::FieldInline("Filter");
                Toolkit::Composer::Combo("##Filter", Entry.Filter);

                Toolkit::Composer::FieldInline("Address");
                Toolkit::Composer::Combo("##Address", Entry.Address);

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
            }

            if (Toolkit::Composer::Button(ICON_FA_PLUS "  Add Texture"))
            {
                mBindings.Append(Binding());
            }

            // Parameters.
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

                    Toolkit::Composer::InputText("##Name", Entry.Name, [&Entry](Text Value)
                    {
                        Entry.Name = Str(Value);
                    });

                    Toolkit::Composer::TableNextColumn();

                    Toolkit::Composer::Combo("##Type", Entry.Type);

                    Toolkit::Composer::TableNextColumn();

                    // A value is edited as what it is, so a boolean toggles and a whole number never drifts.
                    if (Entry.Type == Graphic::Uniform::Bool)
                    {
                        Toolkit::Composer::Checkbox("##Value", Entry.Boolean);
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
                                Toolkit::Composer::DragInt("##Value", Entry.Integer[Component]);
                            }
                            else
                            {
                                Toolkit::Composer::DragFloat("##Value", Entry.Decimal[Component], 0.01f);
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
            }

            if (Toolkit::Composer::Button(ICON_FA_PLUS "  Add Parameter"))
            {
                mConstants.Append(Constant());
            }

            Toolkit::Composer::Separator();

            if (Toolkit::Composer::Button("Save", 96.0f))
            {
                Save();

                Saved   = true;
                Visible = false;
            }

            Toolkit::Composer::SameLine();

            if (Toolkit::Composer::Button("Cancel", 96.0f))
            {
                Visible = false;
            }
        }
        Toolkit::Composer::End();

        if (!Visible)
        {
            mPath = Str();
        }

        // The fields have had their chance to claim a selection, which is what the shared browser expects.
        mBrowser.Draw();

        return Saved;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Materializer::Write(Text Path, ConstSpan<Binding> Bindings, ConstSpan<Constant> Constants)
    {
        JsonValue Document;
        Document.SetObject();

        JsonObject Root     = JsonObject(Document);
        JsonObject Images   = Root.SetObject("Images");
        JsonObject Samplers = Root.SetObject("Samplers");

        for (ConstRef<Binding> Entry : Bindings)
        {
            if (Entry.Name.IsEmpty())
            {
                continue;
            }

            Images.SetObject(Entry.Name).SetString("Path", Entry.Path);

            // Every texture leaves with the sampler that reads it, named the same, as the loader expects.
            JsonObject Sampler = Samplers.SetObject(Entry.Name);
            Sampler.SetEnum("Filter", Entry.Filter);
            Sampler.SetEnum("AddressModeU", Entry.Address);
            Sampler.SetEnum("AddressModeV", Entry.Address);
            Sampler.SetEnum("AddressModeW", Entry.Address);
        }

        if (!Constants.IsEmpty())
        {
            JsonObject Parameters = Root.SetObject("Parameters");

            for (ConstRef<Constant> Entry : Constants)
            {
                if (Entry.Name.IsEmpty())
                {
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
            LOG_E("Materializer: failed to write '{0}'", Path);
            return false;
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Materializer::Save()
    {
        if (!Write(mPath, mBindings, mConstants))
        {
            return;
        }

        // Saving is only half of it; the material in memory is still the one that was read.
        Ref<Content::Service> Service = mContext.GetContent();

        Service.Reload(Service.Load<Graphic::Material>(mKey));
    }
}