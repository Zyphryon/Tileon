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

#include "Assets.hpp"
#include "Tileon.Editor/Context.hpp"
#include "Tileon.Editor/Asset/Editor/MaterialEditor.hpp"
#include "Tileon.Editor/Asset/Editor/TextureEditor.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Assets::Assets(Ref<Context> Context)
        : Panel           { Context, "Assets", true },
          mCreating       { nullptr },
          mCreatingFolder { false },
          mDeletingFolder { false },
          mColumn         { Column::Name },
          mAscending      { true },
          mStep           { 0 },
          mImporter       { Context }
    {
        mEditors.Append(Unique<MaterialEditor>::Create(Context));
        mEditors.Append(Unique<TextureEditor>::Create(Context));

        Refresh();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assets::OnDraw()
    {
        Toolkit::Composer::SetNextWindowSize(720.0f, 260.0f, ImGuiCond_FirstUseEver);

        // The panel is a window of its own, which is what lets the dock space take it.
        if (Toolkit::Composer::Begin(GetTitle(), mVisible))
        {
            DrawTrail();

            Toolkit::Composer::Separator();

            // The listing takes the whole window: what an entry is gets answered on the cursor rather than
            // in a pane that has to be kept beside it. The table is not wrapped in a child, because the
            // folder's context menu binds to the window it is opened from and a child would swallow every
            // right click.
            DrawEntries();

            DrawFolderPrompt();
            DrawDeletePrompt();

            // What an import bakes lands in the folder in view, which is the one this window is showing.
            if (mImporter.DrawPrompt(Resolve(Text())))
            {
                Refresh();
            }
        }
        Toolkit::Composer::End();

        // Each asset is authored in a window of its own, and what one writes is a file in the folder.
        for (ConstRef<Unique<AssetEditor>> Editor : mEditors)
        {
            if (Editor->Draw())
            {
                Refresh();
            }
        }

        mImporter.DrawExplorer();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assets::Refresh()
    {
        mEntries.Clear();

        Filesystem::Enumerate(Resolve(Text()), [this](ConstRef<Filesystem::Record> Record)
        {
            const Bool Folder = (Record.Type == Filesystem::Type::Directory);

            mEntries.Append(
                Str(Record.Name), Folder ? nullptr : mCatalog.Find(Record.Name), Folder, Folder ? 0 : Record.Size);
            return true;
        });

        Sort();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assets::Sort()
    {
        // Names collate without regard to case, so "Assets" and "assets" land next to each other.
        const auto Collate = [](Text Left, Text Right) -> SInt32
        {
            const UInt Length = Min(Left.GetSize(), Right.GetSize());

            for (UInt Index = 0; Index < Length; ++Index)
            {
                const Char First  = StrLowercase(Left[Index]);
                const Char Second = StrLowercase(Right[Index]);

                if (First != Second)
                {
                    return First < Second ? -1 : 1;
                }
            }

            if (Left.GetSize() == Right.GetSize())
            {
                return 0;
            }
            return Left.GetSize() < Right.GetSize() ? -1 : 1;
        };

        const auto Compare = [this, &Collate](ConstRef<Entry> Left, ConstRef<Entry> Right)
        {
            // Folders head the listing whichever way the column is ordered, or the way deeper would sink
            // under the files at this level every time the order is flipped.
            if (Left.Folder != Right.Folder)
            {
                return Left.Folder;
            }

            SInt32 Order = 0;

            if (mColumn == Column::Size)
            {
                Order = (Left.Size == Right.Size) ? 0 : (Left.Size < Right.Size ? -1 : 1);
            }

            // Falling back to the name keeps entries that share a key in an order of their own.
            if (Order == 0)
            {
                Order = Collate(Left.Name, Right.Name);
            }
            return mAscending ? Order < 0 : Order > 0;
        };

        mEntries.Sort(Compare);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assets::Navigate(AnyRef<Str> Folder)
    {
        mFolder    = Move(Folder);
        mSelection.Clear();

        Refresh();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Str Assets::Resolve(Text Name)
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

    Content::Uri Assets::Locate(Text Name)
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

    void Assets::DrawTrail()
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

    void Assets::DrawEntries()
    {
        constexpr ImGuiTableFlags kFlags = ImGuiTableFlags_Sortable
                                         | ImGuiTableFlags_RowBg
                                         | ImGuiTableFlags_BordersInnerV
                                         | ImGuiTableFlags_ScrollY;

        // The row answers across its whole width, or the highlight stops at the name and the listing looks
        // banded rather than picked.
        constexpr ImGuiSelectableFlags kRowFlags = ImGuiSelectableFlags_SpanAllColumns;

        /// The longest side a hovered row shows its art at, in pixels.
        constexpr Real32 kThumbnail = 256.0f;

        if (!Toolkit::Composer::BeginTable("##Entries", 2, kFlags))
        {
            return;
        }

        Toolkit::Composer::TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort);
        Toolkit::Composer::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 96.0f);
        Toolkit::Composer::TableSetupScrollFreeze(0, 1);
        Toolkit::Composer::TableHeadersRow();

        // Adopt whatever ordering the header row asks for.
        if (const Ptr<ImGuiTableSortSpecs> Specs = Toolkit::Composer::TableGetSortSpecs();
            Specs && Specs->SpecsDirty && Specs->SpecsCount > 0)
        {
            mColumn    = static_cast<Column>(Specs->Specs[0].ColumnIndex);
            mAscending = (Specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending);

            Specs->SpecsDirty = false;

            Sort();
        }

        // A folder chosen mid-list would invalidate the rows behind it, so the walk is taken after the table.
        Str Target;

        for (ConstRef<Entry> Item : mEntries)
        {
            Toolkit::Composer::TableNextRow();
            Toolkit::Composer::TableNextColumn();

            Toolkit::Composer::PushID(Item.Name);

            if (Toolkit::Composer::Selectable(
                    String<256>::Print<"{0}  {1}">(GetIcon(Item), Item.Name), mSelection == Item.Name, kRowFlags))
            {
                if (Item.Folder)
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

            // Resting on a row answers what it is; the pane is what the click is for. The delay keeps the
            // art from flashing past every row on the way down the list.
            if (Item.Type && Toolkit::Composer::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            {
                // A row that has only just come under the cursor starts at the front of whatever it holds.
                if (mHovered != Item.Name)
                {
                    mHovered = Item.Name;
                    mStep    = 0;
                }

                // Pinned by its bottom-left corner, so art opened on the last row of a full listing grows
                // up the window rather than off the end of it.
                const ImVec2 Cursor = Toolkit::Composer::GetMousePos();

                Toolkit::Composer::SetNextWindowPos(
                    ImVec2(Cursor.x + 16.0f, Cursor.y - 8.0f), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
                Toolkit::Composer::BeginTooltip();

                const UInt32 Steps = Item.Type->Thumbnail(
                    GetContext().GetContent(), Locate(Item.Name), kThumbnail, mStep);

                if (Steps == 0)
                {
                    Toolkit::Composer::TextDisabled(Item.Type->GetLabel());
                }
                Toolkit::Composer::EndTooltip();

                // A tooltip takes no input of its own, so the wheel is read from the row beneath it. It is
                // only taken away from the listing when there is in fact more than one face to step through,
                // which leaves scrolling past ordinary files exactly as it was.
                if (const Real32 Wheel = Toolkit::Composer::GetMouseWheel(); Wheel != 0.0f && Steps > 1)
                {
                    const SInt32 Next = static_cast<SInt32>(mStep) + (Wheel > 0.0f ? 1 : -1);

                    mStep = static_cast<UInt32>(Clamp<SInt32>(Next, 0, static_cast<SInt32>(Steps) - 1));

                    Toolkit::Composer::ConsumeMouseWheel();
                }
            }

            if (Toolkit::Composer::BeginPopupContextItem())
            {
                DrawEntryMenu(Item);
                Toolkit::Composer::EndPopup();
            }

            Toolkit::Composer::TableNextColumn();

            if (!Item.Folder)
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

    void Assets::DrawEntryMenu(ConstRef<Entry> Item)
    {
        // What a file offers is what its kind says it offers, so nothing here names a kind of its own.
        if (Item.Type)
        {
            if (const Ptr<AssetEditor> Editor = Reach(* Item.Type);
                Editor && Toolkit::Composer::MenuItem(ICON_FA_PEN "  Edit"))
            {
                Editor->Open(Resolve(Item.Name), Locate(Item.Name));
            }

            if (Item.Type->CanReload() && Toolkit::Composer::MenuItem(ICON_FA_ROTATE "  Reload"))
            {
                Item.Type->Reload(GetContext().GetContent(), Locate(Item.Name));
            }
        }

        if (Item.Folder && Toolkit::Composer::MenuItem(ICON_FA_FOLDER_OPEN "  Open"))
        {
            Str Deeper(mFolder);

            if (!Deeper.IsEmpty())
            {
                Deeper.Append('/');
            }
            Deeper.Append(Item.Name);

            Navigate(Move(Deeper));
        }

        Toolkit::Composer::Separator();

        // Nothing goes yet; the prompt is what asks, since the menu closes the moment this is clicked.
        if (Toolkit::Composer::MenuItem(ICON_FA_TRASH "  Delete"))
        {
            mDeleting       = Item.Name;
            mDeletingFolder = Item.Folder;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assets::DrawFolderMenu()
    {
        if (Toolkit::Composer::MenuItem(ICON_FA_FOLDER_PLUS "  New Folder"))
        {
            mCreating       = nullptr;
            mCreatingFolder = true;
            mCreation.Clear();
        }

        for (ConstRef<AssetType> Type : mCatalog.GetTypes())
        {
            if (!Type.HasTrait(AssetType::Trait::Create))
            {
                continue;
            }

            if (Toolkit::Composer::MenuItem(String<64>::Print<"{0}  New {1}">(Type.GetIcon(), Type.GetLabel())))
            {
                mCreating       = AddressOf(Type);
                mCreatingFolder = false;
                mCreation.Clear();
            }
        }

        if (Toolkit::Composer::MenuItem(ICON_FA_ROTATE "  Refresh"))
        {
            Refresh();
        }

        Toolkit::Composer::Separator();

        // A source is chosen from anywhere on disk; what it is decides what bakes it, and everything it
        // becomes lands in the folder in view.
        if (Toolkit::Composer::MenuItem(ICON_FA_FILE_IMPORT "  Import..."))
        {
            mImporter.Browse();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assets::DrawFolderPrompt()
    {
        if (!mCreating && !mCreatingFolder)
        {
            return;
        }

        const String<64> Title = (mCreatingFolder
            ? String<64>("New Folder")
            : String<64>::Print<"New {0}">(mCreating->GetLabel()));

        Toolkit::Composer::OpenPopup(Title);

        if (!Toolkit::Composer::BeginPopupModal(Title))
        {
            return;
        }

        Toolkit::Composer::InputText("##Name", mCreation, [this](Text Value)
        {
            mCreation = Value;
        });

        if (Toolkit::Composer::Button("Create", 96.0f))
        {
            if (!mCreation.IsEmpty())
            {
                if (mCreatingFolder)
                {
                    Filesystem::Make(Resolve(mCreation));
                    Refresh();
                }
                else if (const Ptr<AssetEditor> Editor = Reach(* mCreating))
                {
                    const String<128> Name
                        = String<128>::Print<"{0}{1}">(mCreation, mCreating->GetExtension());

                    // Some editors write a blank asset at once; others wait until they have something to
                    // write, and say so by answering no.
                    if (Editor->Create(Resolve(Name), Locate(Name)))
                    {
                        Refresh();
                    }
                }
            }

            mCreating       = nullptr;
            mCreatingFolder = false;
            Toolkit::Composer::CloseCurrentPopup();
        }

        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::Button("Cancel", 96.0f))
        {
            mCreating       = nullptr;
            mCreatingFolder = false;
            Toolkit::Composer::CloseCurrentPopup();
        }

        Toolkit::Composer::EndPopup();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assets::DrawDeletePrompt()
    {
        if (mDeleting.IsEmpty())
        {
            return;
        }

        Toolkit::Composer::OpenPopup("Delete");

        if (!Toolkit::Composer::BeginPopupModal("Delete"))
        {
            return;
        }

        // A folder takes everything under it, which is worth saying before it happens rather than after.
        Toolkit::Composer::Label(mDeletingFolder
            ? String<160>::Print<"Delete '{0}' and everything inside it?">(mDeleting)
            : String<160>::Print<"Delete '{0}'?">(mDeleting));
        Toolkit::Composer::TextDisabled("This cannot be undone.");
        Toolkit::Composer::Separator();

        if (Toolkit::Composer::Button("Delete", 96.0f))
        {
            // Before the file goes, so the walk of a folder still has a folder to walk.
            Evict(mDeleting, mDeletingFolder);

            const Str Path = Resolve(mDeleting);

            const Filesystem::Result Result
                = (mDeletingFolder ? Filesystem::DeleteAll(Path) : Filesystem::Delete(Path));

            if (Result != Filesystem::Result::Success)
            {
                LOG_E("Assets: failed to delete '{0}'", Path);
            }

            // The details pane reads from the selection, which now names something that is no longer there.
            if (mSelection == mDeleting)
            {
                mSelection.Clear();
            }

            mDeleting.Clear();

            Refresh();
            Toolkit::Composer::CloseCurrentPopup();
        }

        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::Button("Cancel", 96.0f))
        {
            mDeleting.Clear();

            Toolkit::Composer::CloseCurrentPopup();
        }

        Toolkit::Composer::EndPopup();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Assets::Evict(Text Name, Bool Folder)
    {
        // A folder holds nothing of its own; what it holds is loaded under names of its own, so it is walked
        // rather than dropped, and a tree goes the same way one file does.
        if (Folder)
        {
            Filesystem::Enumerate(Resolve(Name), [&](ConstRef<Filesystem::Record> Record)
            {
                Evict(String<256>::Print<"{0}/{1}">(Name, Record.Name),
                      Record.Type == Filesystem::Type::Directory);
                return true;
            });
            return;
        }

        // A cached copy of a file that no longer exists is one nothing can ever refresh, so it goes along.
        if (ConstPtr<AssetType> Type = mCatalog.Find(Name))
        {
            Type->Unload(GetContext().GetContent(), Locate(Name));
        }
    }


    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ptr<AssetEditor> Assets::Reach(ConstRef<AssetType> Type) const
    {
        for (ConstRef<Unique<AssetEditor>> Editor : mEditors)
        {
            if (Editor->GetExtension() == Type.GetExtension())
            {
                return AddressOf(* Editor);
            }
        }
        return nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Text Assets::GetIcon(ConstRef<Entry> Item)
    {
        if (Item.Folder)
        {
            return ICON_FA_FOLDER;
        }
        return Item.Type ? Item.Type->GetIcon() : ICON_FA_FILE;
    }
}