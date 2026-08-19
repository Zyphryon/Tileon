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

#include "Importer.hpp"
#include "Baker/FontBaker.hpp"
#include "Baker/TextureBaker.hpp"
#include "Tileon.Editor/Toolkit/Composer.hpp"
#include "Tileon.Editor/Toolkit/Filter.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool HasSuffix(Text List, Text Name)
    {
        if (List.IsEmpty())
        {
            return true;
        }

        for (UInt Cursor = 0; Cursor < List.GetSize(); )
        {
            const SInt Break  = StrFind(List.Slice(Cursor), ' ');
            const UInt Length = (Break < 0 ? List.GetSize() - Cursor : static_cast<UInt>(Break));

            if (Length > 0 && StrEndsWith(Name, List.Slice(Cursor, Length)))
            {
                return true;
            }
            Cursor += Length + 1;
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Importer::Importer(Ref<Context> Context)
        : mContext { Context },
          mPending { nullptr }
    {
        mBakers.Append(Unique<TextureBaker>::Create(Context));
        mBakers.Append(Unique<FontBaker>::Create(Context));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Importer::Browse()
    {
        // Every baker is offered as a group of its own, with all of them together at the front, the way a
        // system dialog lists its kinds. Whichever file is picked still settles which baker answers for it.
        Sequence<Toolkit::Explorer::Filter> Filters;

        Str Every;

        for (ConstRef<Unique<AssetBaker>> Baker : mBakers)
        {
            if (!Every.IsEmpty())
            {
                Every.Append(' ');
            }
            Every.Append(Baker->GetSources());
        }

        if (mBakers.GetSize() > 1)
        {
            Filters.Append("All Supported", Every);
        }

        for (ConstRef<Unique<AssetBaker>> Baker : mBakers)
        {
            Filters.Append(Baker->GetLabel(), Baker->GetSources());
        }

        mExplorer.Open(Toolkit::Explorer::Mode::Open, Filesystem::GetRootFolder(), Filters, [this](Text Path)
        {
            mImport  = Path;
            mPending = Reach(Path);

            if (!mPending)
            {
                LOG_W("Importer: nothing bakes '{0}'", Path);
            }
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Importer::DrawExplorer()
    {
        mExplorer.Draw();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ptr<AssetBaker> Importer::Reach(Text Source) const
    {
        for (ConstRef<Unique<AssetBaker>> Baker : mBakers)
        {
            if (Toolkit::HasSuffix(Baker->GetSources(), Source))
            {
                return AddressOf(* Baker);
            }
        }
        return nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Importer::DrawPrompt(Text Folder)
    {
        if (!mPending)
        {
            return false;
        }

        // The shell around a bake is the same whatever is being baked, so only the settings inside it and
        // the bake itself come from the baker.
        const String<64> Title = String<64>::Print<"Import {0}">(mPending->GetLabel());

        Toolkit::Composer::OpenPopup(Title);

        if (!Toolkit::Composer::BeginPopupModal(Title))
        {
            return false;
        }

        Toolkit::Composer::TextDisabled(StrAfterLast(mImport, '/'));
        Toolkit::Composer::Separator();

        mPending->DrawSettings();

        Toolkit::Composer::Separator();

        Bool Baked = false;

        if (Toolkit::Composer::Button("Import", 96.0f))
        {
            Baked    = mPending->Bake(mImport, Folder);
            mPending = nullptr;

            Toolkit::Composer::CloseCurrentPopup();
        }

        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::Button("Cancel", 96.0f))
        {
            mPending = nullptr;

            Toolkit::Composer::CloseCurrentPopup();
        }

        Toolkit::Composer::EndPopup();

        return Baked;
    }
}