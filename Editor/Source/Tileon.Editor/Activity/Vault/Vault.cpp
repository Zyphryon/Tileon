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

#include "Vault.hpp"
#include "Tileon.Editor/Context.hpp"
#include <Zyphryon.Graphic/Material.hpp>
#include <Zyphryon.Graphic/Technique.hpp>
#include <Zyphryon.Render/2D/Font.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Vault::Vault(Ref<Context> Context)
        : Activity  { Context, "Content", true },
          mCreating { Creation::None },
          mImporter { Context },
          mGlaze    { Context }
    {
        Refresh();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Vault::OnDraw()
    {
        Toolkit::Composer::SetNextWindowSize(720.0f, 260.0f, ImGuiCond_FirstUseEver);

        // The panel is a window of its own, which is what lets the dock space take it.
        if (Toolkit::Composer::Begin(GetTitle(), mVisible))
        {
            DrawTrail();

            Toolkit::Composer::Separator();

            DrawEntries();
            DrawFolderPrompt();

            // What an import bakes lands in the folder in view, which is the one this window is showing.
            if (mImporter.DrawPrompt(Resolve(Text())))
            {
                Refresh();
            }
        }
        Toolkit::Composer::End();

        // A material is authored in a window of its own, and what it writes is one more file in the folder.
        if (mGlaze.Draw())
        {
            Refresh();
        }

        mImporter.DrawBrowser();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Vault::Kind Vault::Classify(Text Name)
    {
        const Text Extension = StrAfterLast(Name, '.');

        if (Extension == "mtl")
        {
            return Kind::Material;
        }
        if (Extension == "fnt")
        {
            return Kind::Font;
        }
        if (Extension == "vfx")
        {
            return Kind::Technique;
        }
        if (Extension == "tex")
        {
            return Kind::Texture;
        }
        if (Extension == "png" || Extension == "jpg" || Extension == "ttf" || Extension == "otf")
        {
            return Kind::Source;
        }
        return Kind::Other;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Text Vault::GetIcon(Kind Kind)
    {
        switch (Kind)
        {
        case Kind::Folder:
            return ICON_FA_FOLDER;
        case Kind::Material:
            return ICON_FA_PALETTE;
        case Kind::Font:
            return ICON_FA_FONT;
        case Kind::Technique:
            return ICON_FA_WAND_MAGIC_SPARKLES;
        case Kind::Texture:
            return ICON_FA_IMAGE;
        case Kind::Source:
            return ICON_FA_FILE_IMPORT;
        default:
            return ICON_FA_FILE;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Vault::Refresh()
    {
        mEntries.Clear();

        const Str Folder = Resolve(Text());

        // Two passes, so folders lead and the way deeper is never buried under the files at this level.
        Filesystem::Enumerate(Folder, [this](ConstRef<Filesystem::Record> Record)
        {
            if (Record.Type == Filesystem::Type::Directory)
            {
                mEntries.Append(Entry(Str(Record.Name), Kind::Folder, 0));
            }
            return true;
        });

        Filesystem::Enumerate(Folder, [this](ConstRef<Filesystem::Record> Record)
        {
            if (Record.Type != Filesystem::Type::Directory)
            {
                mEntries.Append(Entry(Str(Record.Name), Classify(Record.Name), Record.Size));
            }
            return true;
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Vault::Navigate(AnyRef<Str> Folder)
    {
        mFolder    = Move(Folder);
        mSelection = Str();

        Refresh();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Vault::Resolve(Text Name)
    {
        Str Path(GetContext().GetProject().GetFolder());

        if (!mFolder.IsEmpty())
        {
            Path.Append('/');
            Path.Append(mFolder);
        }

        if (!Name.IsEmpty())
        {
            Path.Append('/');
            Path.Append(Name);
        }
        return Path;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Content::Uri Vault::Locate(Text Name)
    {
        // The mount answers for the project's folder, so an asset is named by where it sits inside it.
        Str Path("Resources://");

        if (!mFolder.IsEmpty())
        {
            Path.Append(mFolder);
            Path.Append('/');
        }
        Path.Append(Name);

        return Content::Uri(Move(Path));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Vault::DrawTrail()
    {
        if (Toolkit::Composer::Button(ICON_FA_HOUSE "##Root"))
        {
            Navigate(Str());
        }

        // Every folder on the way here is a way back, so each one is drawn as a button of its own.
        Str Walked;
        Str Target;

        StrSplit(mFolder, '/', [&](Text Step)
        {
            if (Step.IsEmpty())
            {
                return true;
            }

            if (!Walked.IsEmpty())
            {
                Walked.Append('/');
            }
            Walked.Append(Step);

            Toolkit::Composer::SameLine();

            if (Toolkit::Composer::Button(String<128>::Print<"{0}##Trail">(Step)))
            {
                Target = Walked;
            }
            return true;
        });

        if (!Target.IsEmpty())
        {
            Navigate(Move(Target));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Vault::DrawEntries()
    {
        constexpr ImGuiTableFlags kFlags = ImGuiTableFlags_RowBg
                                         | ImGuiTableFlags_BordersInnerV
                                         | ImGuiTableFlags_ScrollY;

        if (!Toolkit::Composer::BeginTable("##Entries", 2, kFlags))
        {
            return;
        }

        Toolkit::Composer::TableSetupColumn("Name");
        Toolkit::Composer::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 96.0f);

        // A folder chosen mid-list would invalidate the rows behind it, so the walk is taken after the table.
        Str Target;

        for (ConstRef<Entry> Item : mEntries)
        {
            Toolkit::Composer::TableNextRow();
            Toolkit::Composer::TableNextColumn();

            Toolkit::Composer::PushID(Item.Name);

            if (Toolkit::Composer::Selectable(
                    String<256>::Print<"{0}  {1}">(GetIcon(Item.Kind), Item.Name), mSelection == Item.Name))
            {
                if (Item.Kind == Kind::Folder)
                {
                    Target = mFolder;

                    if (!Target.IsEmpty())
                    {
                        Target.Append('/');
                    }
                    Target.Append(Item.Name);
                }
                else
                {
                    mSelection = Item.Name;
                }
            }

            if (Toolkit::Composer::BeginPopupContextItem())
            {
                DrawEntryMenu(Item);
                Toolkit::Composer::EndPopup();
            }

            Toolkit::Composer::TableNextColumn();

            if (Item.Kind != Kind::Folder)
            {
                Toolkit::Composer::TextDisabled(String<32>::Print<"{0} KB">(1 + Item.Size / 1024));
            }

            Toolkit::Composer::PopID();
        }

        Toolkit::Composer::EndTable();

        // The folder answers a right click only where no row does, or it would shadow every entry's own menu.
        if (Toolkit::Composer::BeginPopupContextWindow("##Folder"))
        {
            DrawFolderMenu();
            Toolkit::Composer::EndPopup();
        }

        if (!Target.IsEmpty())
        {
            Navigate(Move(Target));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Vault::DrawEntryMenu(ConstRef<Entry> Item)
    {
        // Reloading is the one action every baked asset understands, and the only one some of them do.
        const Bool Reloadable = (Item.Kind == Kind::Material)
                             || (Item.Kind == Kind::Font)
                             || (Item.Kind == Kind::Technique)
                             || (Item.Kind == Kind::Texture);

        // A material is authored here rather than by hand, since its names are hashed the moment it loads.
        if (Item.Kind == Kind::Material && Toolkit::Composer::MenuItem(ICON_FA_PEN "  Edit"))
        {
            mGlaze.Open(Resolve(Item.Name), Locate(Item.Name));
        }

        if (Reloadable && Toolkit::Composer::MenuItem(ICON_FA_ROTATE "  Reload"))
        {
            Reload(Item);
        }

        if (Item.Kind == Kind::Folder && Toolkit::Composer::MenuItem(ICON_FA_FOLDER_OPEN "  Open"))
        {
            Str Deeper(mFolder);

            if (!Deeper.IsEmpty())
            {
                Deeper.Append('/');
            }
            Deeper.Append(Item.Name);

            Navigate(Move(Deeper));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Vault::DrawFolderMenu()
    {
        if (Toolkit::Composer::MenuItem(ICON_FA_FOLDER_PLUS "  New Folder"))
        {
            mCreating = Creation::Folder;
            mCreation = Str();
        }

        if (Toolkit::Composer::MenuItem(ICON_FA_PALETTE "  New Material"))
        {
            mCreating = Creation::Material;
            mCreation = Str();
        }

        if (Toolkit::Composer::MenuItem(ICON_FA_ROTATE "  Refresh"))
        {
            Refresh();
        }

        Toolkit::Composer::Separator();

        // A source is chosen from anywhere on disk; everything it becomes lands in the folder in view.
        if (Toolkit::Composer::MenuItem(ICON_FA_FILE_IMPORT "  Import Font..."))
        {
            mImporter.Browse(Importer::Kind::Font);
        }

        if (Toolkit::Composer::MenuItem(ICON_FA_FILE_IMPORT "  Import Texture..."))
        {
            mImporter.Browse(Importer::Kind::Texture);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Vault::DrawFolderPrompt()
    {
        if (mCreating == Creation::None)
        {
            return;
        }

        const Text Title = (mCreating == Creation::Folder) ? "New Folder"_Text : "New Material"_Text;

        Toolkit::Composer::OpenPopup(Title);

        if (!Toolkit::Composer::BeginPopupModal(Title))
        {
            return;
        }

        Toolkit::Composer::InputText("##Name", mCreation, [this](Text Value)
        {
            mCreation = Str(Value);
        });

        if (Toolkit::Composer::Button("Create", 96.0f))
        {
            if (!mCreation.IsEmpty())
            {
                if (mCreating == Creation::Folder)
                {
                    Filesystem::Make(Resolve(mCreation));
                    Refresh();
                }
                else
                {
                    const String<128> Name = String<128>::Print<"{0}.mtl">(mCreation);

                    if (mGlaze.Create(Resolve(Name), Locate(Name)))
                    {
                        Refresh();
                    }
                }
            }

            mCreating = Creation::None;
            Toolkit::Composer::CloseCurrentPopup();
        }

        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::Button("Cancel", 96.0f))
        {
            mCreating = Creation::None;
            Toolkit::Composer::CloseCurrentPopup();
        }

        Toolkit::Composer::EndPopup();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Vault::Reload(ConstRef<Entry> Item)
    {
        Ref<Content::Service> Service = GetContext().GetContent();

        const Content::Uri Key = Locate(Item.Name);

        switch (Item.Kind)
        {
        case Kind::Material:
            Service.Reload(Service.Load<Graphic::Material>(Key));
            break;
        case Kind::Font:
            Service.Reload(Service.Load<::Render::Font>(Key));
            break;
        case Kind::Technique:
            Service.Reload(Service.Load<Graphic::Technique>(Key));
            break;
        case Kind::Texture:
            Service.Reload(Service.Load<Graphic::Image>(Key));
            break;
        default:
            break;
        }
    }
}