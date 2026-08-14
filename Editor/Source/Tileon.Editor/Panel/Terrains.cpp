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

#include "Terrains.hpp"
#include "Tileon.Editor/Utility.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Terrains::Terrains(Ref<Context> Context)
        : Panel       { Context, "Terrains" },
          mRepository { Context.GetRepository() },
          mTileset    { Context.GetTileset() },
          mScheduler  { Context.GetScheduler() },
          mSelection  { 0 },
          mScroll     { 0 },
          mBrowser    { Context.GetContent() },
          mAuthored   { 0 },
          mMeasured   { false },
          mOriginX    { 0 },
          mOriginY    { 0 },
          mWidth      { 0 },
          mHeight     { 0 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrains::OnDraw()
    {
        // Adopt a terrain requested from another panel (Palette right-click), scroll it into view, and pull focus.
        if (const SInt64 Request = GetContext().GetInteger("Selection.Tile.Target", 0); Request != 0)
        {
            mSelection = static_cast<UInt16>(Request);
            mScroll    = static_cast<UInt16>(Request);
            mPreview.Reset();

            GetContext().SetInteger("Selection.Tile.Target", 0);
            Toolkit::Composer::SetNextWindowFocus();
        }

        Toolkit::Composer::SetNextWindowSize(1160.0f, 680.0f, ImGuiCond_FirstUseEver);
        Toolkit::Composer::SetNextWindowSizeConstraints(900.0f, 500.0f, 1800.0f, 1400.0f);

        if (Toolkit::Composer::Begin(GetTitle(), mVisible))
        {
            const Real32 Padding = -(Toolkit::Composer::GetFrameHeightWithSpacing() + 8.0f);

            Toolkit::Composer::BeginChild("##list_panel", ImVec2(180.0f, Padding),
                ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders);
            DrawListPanel();
            Toolkit::Composer::EndChild();

            if (mRepository.HasTerrain(mSelection))
            {
                Ref<Terrain> Terrain = mRepository.GetTerrain(mSelection);
                Ref<Motif>   Motif   = mTileset.GetMotif(mSelection);

                Toolkit::Composer::SameLine();
                Toolkit::Composer::BeginChild("##left_panel", ImVec2(420.0f, Padding), ImGuiChildFlags_Borders);
                DrawLeftPanel(Terrain, Motif);
                Toolkit::Composer::EndChild();

                Toolkit::Composer::SameLine();
                Toolkit::Composer::BeginChild("##right_panel", ImVec2(0.0f, Padding), ImGuiChildFlags_Borders);

                DrawRightPanel(Motif);

                Toolkit::Composer::EndChild();
            }
            else
            {
                Toolkit::Composer::SameLine();
                Toolkit::Composer::BeginChild("##empty_panel", ImVec2(0.0f, Padding), ImGuiChildFlags_Borders);
                DrawEmptyPanel("Select a terrain to start editing", "?");
                Toolkit::Composer::EndChild();
            }

            DrawStatusBar();
        }
        Toolkit::Composer::End();

        // The fields claim their selection before this point, which is what the shared browser expects.
        mBrowser.Draw();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrains::DrawListPanel()
    {
        const Bool WasPlusClicked = Toolkit::Composer::Button("+", -1.0f);

        if (WasPlusClicked)
        {
            Ref<Terrain> Terrain = mRepository.CreateTerrain();

            mSelection = Terrain.GetID();
        }

        Toolkit::Composer::Separator();
        Toolkit::Composer::BeginChild("##list_scroll", ImVec2(0.0f, 0.0f));

        mRepository.ForEachTerrain([&](ConstRef<Terrain> Terrain)
        {
            const Bool Selected = (mSelection == Terrain.GetID());

            Toolkit::Composer::Selectable(String<256>::Print<"{0:04} {1}">(Terrain.GetID(), Terrain.GetName()), Selected);

            if (Toolkit::Composer::IsItemClicked())
            {
                if (mSelection != Terrain.GetID())
                {
                    mSelection = Terrain.GetID();

                    mPreview.Reset();
                }
            }

            if (Selected && WasPlusClicked)
            {
                Toolkit::Composer::SetScrollHereY(0.5f);
            }

            // Bring a freshly cloned terrain into view once, then clear the request.
            if (mScroll == Terrain.GetID())
            {
                mScroll = 0;
                Toolkit::Composer::SetScrollHereY(0.5f);
            }

            if (Toolkit::Composer::BeginPopupContextItem())
            {
                if (Toolkit::Composer::MenuItem("Clone"))
                {
                    Ref<Tileon::Terrain> Clone = mRepository.CloneTerrain(Terrain.GetID());
                    mTileset.Clone(Terrain.GetID(), Clone.GetID());

                    mSelection = Clone.GetID();
                    mScroll    = Clone.GetID();
                    mPreview.Reset();
                }

                Toolkit::Composer::Separator();

                if (Toolkit::Composer::MenuItem("Delete"))
                {
                    Discard(GetContext().GetProject().GetFolder(), Terrain.GetID());

                    mRepository.DeleteTerrain(Terrain.GetID());
                }
                Toolkit::Composer::EndPopup();
            }
        });

        Toolkit::Composer::EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrains::DrawLeftPanel(Ref<Terrain> Terrain, Ref<Motif> Motif)
    {
        // Draw the editable fields for the terrain identity properties.
        Toolkit::Composer::Section("Identity");

        Toolkit::Composer::Field("Name");
        Toolkit::Composer::PushItemWidth(-1);
        Toolkit::Composer::InputText("##name", Terrain.GetName(), [&](Text Value)
        {
            Terrain.SetName(Value);
        });
        Toolkit::Composer::PopItemWidth();
        Toolkit::Composer::Spacing();

        Toolkit::Composer::Field("Color");
        Toolkit::Composer::PushItemWidth(-1);
        IntColor8 Tint = Motif.GetTint();
        if (Toolkit::Composer::InputTintSmall("##tint", Tint))
        {
            Motif.SetTint(Tint);
        }
        Toolkit::Composer::PopItemWidth();
        Toolkit::Composer::Spacing();

        DrawLeftPanelArt(Motif);

        // Draw the animation section for the motif.
        Toolkit::Composer::Section("Animation");

        Toolkit::Composer::Field("Easing");
        Toolkit::Composer::PushItemWidth(-1);
        if (Toolkit::Composer::BeginCombo("##easing", Enum::GetName(Motif.GetEasing())))
        {
            for (const Easing Option : Enum::GetValues<Easing>())
            {
                const Bool Selected = (Motif.GetEasing() == Option);

                if (Toolkit::Composer::Selectable(Enum::GetName(Option), Selected))
                {
                    Motif.SetEasing(Option);
                }
            }
            Toolkit::Composer::EndCombo();
        }
        Toolkit::Composer::PopItemWidth();
        Toolkit::Composer::Spacing();

        Toolkit::Composer::Field("Frames");
        DrawLeftPanelAnimation(Motif);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrains::DrawLeftPanelArt(Ref<Motif> Motif)
    {
        constexpr UInt64 kSourceKey = "Terrains.Source"_Hash;

        Toolkit::Composer::Section("Art");

        ConstRef<Str> Folder = GetContext().GetProject().GetFolder();

        // The fields describe one motif at a time, so they say what it already holds until they are typed over.
        if (mAuthored != Motif.GetID())
        {
            mAuthored = Motif.GetID();
            mFault    = Str();
            mSource   = Motif.GetOrigin().GetUrl();
            mMeasured = false;
            mOriginX  = 0;
            mOriginY  = 0;
            mWidth    = 0;
            mHeight   = 0;
        }

        if (Str Selection; mBrowser.Consume(kSourceKey, Selection))
        {
            mSource   = Move(Selection);
            mMeasured = false;
            mWidth    = 0;
            mHeight   = 0;
        }

        // The art is loaded as soon as it is named, which is what lets the extent follow from it.
        if (!mArt || mArt->GetKey().GetUrl() != mSource)
        {
            mArt = mSource.IsEmpty()
                ? Retainer<Graphic::Image>()
                : GetContext().GetContent().Load<Graphic::Image>(Content::Uri(mSource));
        }

        Toolkit::Composer::Field("Source");
        Toolkit::Composer::InputTextWithButton("##source", mSource,
            [&](Text Value)
            {
                mSource = Str(Value);
            },
            "...",
            [&]
            {
                mBrowser.Open(kSourceKey, ".tex");
            },
            ImGuiInputTextFlags_EnterReturnsTrue);
        Toolkit::Composer::Spacing();

        Text   Baked = Text();
        UInt16 Cut   = 0;
        UInt16 Atlas = 0;

        mTileset.ForEachPlacement([&](ConstRef<Tileset::Placement> Placement)
        {
            if (Placement.Motif == Motif.GetID())
            {
                Baked = mTileset.GetAtlas(Placement.Atlas)
                    ? mTileset.GetAtlas(Placement.Atlas)->GetKey().GetUrl()
                    : Text();
                Cut   = Placement.Frames;
                Atlas = Placement.Atlas;
            }
        });

        // A motif takes a whole run at a time, so the frames are authored on a copy and handed back below.
        Motif::Flipbook Flipbook = Motif.GetFlipbook();
        Bool            Authored = false;

        // An authored terrain is not measured at all: its extent is whatever its sheet was cut with.
        if (!mMeasured && mWidth == 0 && Cut > 0)
        {
            if (ConstRetainer<Graphic::Image> Sheet = mTileset.GetAtlas(Atlas); Sheet && Sheet->HasFinished())
            {
                mWidth    = Sheet->GetWidth();
                mHeight   = Sheet->GetHeight();
                mMeasured = true;
            }
        }

        // Whatever the terrain is, an unknown extent falls back to the art rather than staying unusable.
        if (!mMeasured && mWidth == 0 && mHeight == 0 && mArt && mArt->HasFinished())
        {
            LOG_W("Terrains: taking {0}x{1} from '{2}' as the extent of motif {3}",
                mArt->GetWidth(), mArt->GetHeight(), mSource, Motif.GetID());

            mMeasured = true;

            // A frame is as tall as the art and as wide as it is tall, until it is told otherwise.
            mHeight = mArt->GetHeight();
            mWidth  = mHeight;

            // Frames sit side by side, so the art holds as many as it is wide.
            Flipbook.Clear();

            for (UInt16 Frame = Max<UInt16>(mArt->GetWidth() / mWidth, 1); Frame > 0; --Frame)
            {
                Flipbook.Insert(1.0f);
            }

            Authored = true;
        }

        // A run is never empty, so a motif whose art has yet to arrive still holds one frame.
        if (Flipbook.IsEmpty())
        {
            Flipbook.Insert(1.0f);

            Authored = true;
        }

        // A frame covers whole tiles, so its extent steps by the tiles it is measured in.
        const UInt16 Step = static_cast<UInt16>(Max(GetContext().GetDirector().GetDensity(), 1.0f));

        Toolkit::Composer::Field("Frames");
        Toolkit::Composer::Label("{0}", Flipbook.GetCount());

        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::DisabledButton("+", Flipbook.IsFull(), 32.0f))
        {
            Flipbook.Insert(1.0f);

            mMeasured = true;
            Authored  = true;
        }

        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::DisabledButton("-", Flipbook.GetCount() <= 1, 32.0f))
        {
            Flipbook.Remove(Flipbook.GetCount() - 1);

            mMeasured = true;
            Authored  = true;
        }
        Toolkit::Composer::Spacing();

        if (Authored)
        {
            Motif.SetFlipbook(Motif::Flipbook(Flipbook));
        }

        Toolkit::Composer::Field("Origin");
        if (Toolkit::Composer::InputIntPair("##origin", mOriginX, mOriginY, ",", Step, Step))
        {
            mMeasured = true;
        }
        Toolkit::Composer::Spacing();

        Toolkit::Composer::Field("Extent");
        if (Toolkit::Composer::InputIntPair("##extent", mWidth, mHeight, "x", Step, Step))
        {
            mMeasured = true;
        }
        Toolkit::Composer::Spacing();

        // A sheet holds one extent, so art of a new size lands in a sheet of its own without being asked.
        const Str Sheet = Str::Print<kSheet>(mWidth, mHeight);

        Toolkit::Composer::TextDisabled(StrAfterLast(Sheet, '/'));
        Toolkit::Composer::Spacing();

        const Bool Ready = !mSource.IsEmpty() && Flipbook.GetCount() > 0 && mWidth > 0;

        // A bake is pending while what the motif asks for differs from what its run was cut with.
        const Bool Pending = Ready
            && (Cut != Flipbook.GetCount() || Baked != Sheet || Motif.GetOrigin().GetUrl() != mSource);

        if (Toolkit::Composer::DisabledButton(Pending ? "Bake (Pending)"_Text : "Bake"_Text, !Ready, -1.0f))
        {
            // A browser hands back a url while a typed name is bare, so both are taken to the mount first.
            const Content::Uri Target = Content::Uri(Sheet);
            const Content::Uri Origin = Content::Uri(mSource).HasSchema()
                ? Content::Uri(Str(mSource))
                : Content::Uri(Str::Print<"Resources://{0}">(mSource));

            // The baker reads from disk, which is where the mount roots the url it was given.
            const Str Art = Str::Print<"{0}/{1}">(Folder, Origin.GetPath());

            // A frame covers as many tiles as its extent holds, which is what the span means.
            const Real32 Density = GetContext().GetDirector().GetDensity();

            Motif.SetPeriod(IntVector2(
                static_cast<SInt32>(Round(mWidth  / Density)),
                static_cast<SInt32>(Round(mHeight / Density))));

            // The art is remembered so the motif can be fired again without naming it a second time.
            Motif.SetOrigin(Content::Uri(Origin));

            const Bool Fired = Adopt(
                Folder, Motif.GetID(), Target.GetUrl(), Art, static_cast<UInt16>(Flipbook.GetCount()),
                IntVector2(mOriginX, mOriginY), mWidth, mHeight);

            // A bake that fails says so where it was asked for, rather than only in the log.
            mFault = Fired ? Str() : Str("Could not cut every frame from the source. See the log.");
        }

        if (!Ready)
        {
            Toolkit::Composer::TextDisabled(mSource.IsEmpty()
                ? "Name the art this terrain is drawn from."_Text
                : "Waiting for the art to load, or set its extent below."_Text);
        }

        if (!mFault.IsEmpty())
        {
            Toolkit::Composer::Spacing();
            Toolkit::Composer::TextColored(ImVec4(0.90f, 0.30f, 0.30f, 1.0f), mFault);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrains::DrawLeftPanelAnimation(Ref<Motif> Motif)
    {
        ConstRef<Tileset::Glyph> Glyph = mTileset.GetGlyph(Motif.GetID());

        // A frame is a slice of the sheet the motif was fired into, so the run says how many there are.
        if (Glyph.Count == 0)
        {
            constexpr Text kHint = "No Frames";

            Toolkit::Composer::SetCursorPosX((Toolkit::Composer::GetContentRegionAvail().x - Toolkit::Composer::CalcTextSize(kHint).x) * 0.5f);
            Toolkit::Composer::TextDisabled(kHint);
            return;
        }

        if (Toolkit::Composer::BeginTable("##frames", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV))
        {
            Toolkit::Composer::TableSetupColumn("Slice", ImGuiTableColumnFlags_WidthFixed, 64.0f);
            Toolkit::Composer::TableSetupColumn("Duration");
            Toolkit::Composer::TableHeadersRow();

            // A run authored since the last firing may be shorter than the sheet, so the rows stop at the shorter one.
            const UInt16 Count = Min<UInt16>(Glyph.Count, Motif.GetFlipbook().GetCount());

            for (UInt16 Keyframe = 0; Keyframe < Count; ++Keyframe)
            {
                Toolkit::Composer::TableNextRow();
                Toolkit::Composer::TableNextColumn();
                Toolkit::Composer::PushID(Keyframe);

                Toolkit::Composer::Label("{0}", Glyph.Start + Keyframe);

                Toolkit::Composer::TableNextColumn();
                Toolkit::Composer::SetNextItemWidth(-1);

                Real32 Duration = Motif.GetFlipbook().GetDuration(Keyframe);

                if (Toolkit::Composer::DragFloat("##duration", Duration, 0.01f))
                {
                    // A motif takes a whole run at a time, so the edited frame rides back in on a copy.
                    Motif::Flipbook Flipbook = Motif.GetFlipbook();

                    Flipbook.SetDuration(Keyframe, Duration);

                    Motif.SetFlipbook(Move(Flipbook));
                }

                Toolkit::Composer::PopID();
            }

            Toolkit::Composer::EndTable();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrains::DrawRightPanel(ConstRef<Motif> Motif)
    {
        ConstRef<Tileset::Glyph> Glyph = mTileset.GetGlyph(Motif.GetID());

        const Bool Settled = mArt && mArt->HasFinished();

        if (Glyph.Texture == 0 && !Settled)
        {
            DrawEmptyPanel(mSource.IsEmpty()
                ? "No art assigned to this terrain"_Text
                : "Loading the art..."_Text, "?");
            return;
        }

        if (Toolkit::Composer::BeginTabBar("##right_tabs"))
        {
            // The preview shows the tile as the world draws it, which is one slice of the array it was promoted into.
            if (Glyph.Texture)
            {
                if (Toolkit::Composer::BeginTabItem("Preview"))
                {
                    const Real32  Density = GetContext().GetDirector().GetDensity();
                    const Vector2 Size(Glyph.Period.GetX() * Density, Glyph.Period.GetY() * Density);

                    // The scene may be paused while a run is authored, so the preview keeps its own time.
                    const UInt32 Keyframe = Motif.GetFlipbook().Locate(
                        Toolkit::Composer::GetTime(), Motif.GetEasing());
                    const UInt16 Slice    = Glyph.Start + Min<UInt16>(Keyframe, Glyph.Count - 1);

                    // Every slice shares one identifier, so which of them is drawn rides in the coordinates.
                    const Real32 Offset = Slice * Plugin::ImGuiRenderer::kSliceStride;

                    mPreview.Draw(
                        Plugin::ImGuiRenderer::GetLayeredTextureID(Glyph.Texture),
                        Size, Rect(Offset, 0.0f, Offset + 1.0f, 1.0f), Color::FromColor8(Glyph.Tint));

                    Toolkit::Composer::EndTabItem();
                }
            }

            if (Settled)
            {
                if (Toolkit::Composer::BeginTabItem("Source"))
                {
                    const Vector2 Size(mArt->GetWidth(), mArt->GetHeight());

                    // A source holding many slices shows the first, since that is where a run starts.
                    if (mArt->GetLayers() > 1)
                    {
                        mInspector.Draw(
                            Plugin::ImGuiRenderer::GetLayeredTextureID(mArt->GetHandle()),
                            Size, Rect(0.0f, 0.0f, 1.0f, 1.0f));
                    }
                    else
                    {
                        mInspector.Draw(mArt->GetHandle(), Size, Rect::One());
                    }

                    Toolkit::Composer::EndTabItem();
                }
            }

            Toolkit::Composer::EndTabBar();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrains::DrawStatusBar()
    {
        DrawBottomBar("##status_bar", [&](Real32 PadY)
        {
            if (mRepository.HasTerrain(mSelection))
            {
                ConstRef<Terrain>        Terrain = mRepository.GetTerrain(mSelection);
                ConstRef<Motif>          Motif   = mTileset.GetMotif(Terrain.GetID());
                ConstRef<Tileset::Glyph> Glyph   = mTileset.GetGlyph(Motif.GetID());

                Toolkit::Composer::SetCursorPosX(Toolkit::Composer::GetStyle().ItemSpacing.x);
                Toolkit::Composer::Label("{0:04}  {1}", Terrain.GetID(), Terrain.GetName().IsEmpty()
                    ? "(Unnamed)"
                    : Terrain.GetName());

                constexpr Text kStatusLabel[] = {
                    "[--] Empty",
                    "[..] Loading",
                    "[OK] Ready",
                    "[!!] Failed"
                };
                constexpr ImVec4    kStatusColor[] = {
                    ImVec4(0.55f, 0.55f, 0.55f, 1.0f),
                    ImVec4(0.95f, 0.80f, 0.25f, 1.0f),
                    ImVec4(0.35f, 0.85f, 0.45f, 1.0f),
                    ImVec4(0.90f, 0.30f, 0.30f, 1.0f)
                };

                const UInt32 Status  = (Glyph.Count == 0) ? 0u : (Glyph.Texture ? 2u : 1u);
                const Real32 StatusW = Toolkit::Composer::CalcTextSize(kStatusLabel[Status]).x + Toolkit::Composer::GetStyle().ItemSpacing.x * 2.0f;

                Toolkit::Composer::SameLine(Toolkit::Composer::GetWindowWidth() - StatusW);
                Toolkit::Composer::SetCursorPosY(PadY);
                Toolkit::Composer::TextColored(kStatusColor[Status], kStatusLabel[Status]);
            }
            else
            {
                constexpr Text Hint = "No terrain selected";

                Toolkit::Composer::SetCursorPosX((Toolkit::Composer::GetWindowWidth() - Toolkit::Composer::CalcTextSize(Hint).x) * 0.5f);
                Toolkit::Composer::TextDisabled(Hint);
            }
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Terrains::Adopt(
        Text       Folder,
        UInt16     Motif,
        Text       Sheet,
        Text       Source,
        UInt16     Frames,
        IntVector2 Origin,
        UInt16     Width,
        UInt16     Height)
    {
        Change Change;
        Change.Motif  = Motif;
        Change.Sheet  = mTileset.GetOrInsertAtlas(Content::Uri(Str(Sheet)));
        Change.Fired  = true;
        Change.Source = Str(Source);
        Change.Frames = Frames;
        Change.Origin = Origin;
        Change.Width  = Width;
        Change.Height = Height;

        return Fire(Folder, Change);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Terrains::Discard(Text Folder, UInt16 Motif)
    {
        Change Change;
        Change.Motif = Motif;

        return Fire(Folder, Change);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Terrains::Fire(Text Folder, ConstRef<Change> Change)
    {
        Sequence<Batch>              Batches;
        Sequence<Tileset::Placement> Placements;

        // A motif only disturbs the atlas it leaves and the one it joins; every other keeps its runs as they were.
        constexpr UInt32 kNone   = ~0u;
        UInt32           Vacated = kNone;

        mTileset.ForEachPlacement([&](ConstRef<Tileset::Placement> Placement)
        {
            if (Placement.Motif == Change.Motif)
            {
                Vacated = Placement.Atlas;
            }
        });

        Gather(Folder, Change, Batches, Placements);

        // Tile art is authored in sRGB, so it fires to an sRGB format and the GPU decodes it on sample.
        ::Pipeline::Baker::Texture::Profile Settings;
        Settings.Linear  = false;
        Settings.Mipmaps = true;

        const ::Pipeline::Baker::Texture::Baker Assembler(mScheduler);

        for (UInt32 Index = 0; Index < Batches.GetSize(); ++Index)
        {
            // Cutting a atlas that nothing moved through would write the very same slices back over it.
            if (Index != Vacated && !(Change.Fired && Index == Change.Sheet))
            {
                continue;
            }

            const Str Path = Str::Print<"{0}/{1}">(
                Folder, mTileset.GetAtlas(static_cast<UInt16>(Index))->GetKey().GetPath());

            // A atlas nothing is fired into is left where it is, since no run names it any more.
            if (Batches[Index].Entries.IsEmpty())
            {
                continue;
            }

            const Blob Output = Assembler.Assemble(Batches[Index].Entries, Settings);

            if (Output == nullptr)
            {
                LOG_E("Terrains: failed to assemble '{0}'", Path);
                return false;
            }

            Filesystem::Ensure(Path);

            if (Filesystem::Write(Path, Output) != Filesystem::Result::Success)
            {
                LOG_E("Terrains: failed to write '{0}'", Path);
                return false;
            }
        }

        // The atlass and the runs that name them are replaced together, so neither can outlive the other.
        mTileset.Rebind(Move(Placements));
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Terrains::Gather(
        Text                              Folder,
        ConstRef<Change>                  Change,
        Ref<Sequence<Batch>>              Batches,
        Ref<Sequence<Tileset::Placement>> Placements) const
    {
        mTileset.ForEachPlacement([&](ConstRef<Tileset::Placement> Placement)
        {
            // The motif being changed is gathered from what it is taking on, not from what it held.
            if (Placement.Motif == Change.Motif)
            {
                return;
            }

            ConstRetainer<Graphic::Image> Atlas = mTileset.GetAtlas(Placement.Atlas);

            if (!Atlas || !Atlas->HasFinished())
            {
                LOG_W("Terrains: atlas {0} has not settled, so motif {1} keeps its run untouched",
                    Placement.Atlas, Placement.Motif);
                return;
            }

            if (Batches.GetSize() <= Placement.Atlas)
            {
                Batches.Resize(Placement.Atlas + 1);
            }

            Ref<Batch> Destination = Batches[Placement.Atlas];

            Ref<Tileset::Placement> Assigned = Placements.Append();
            Assigned.Motif  = Placement.Motif;
            Assigned.Base   = static_cast<UInt16>(Destination.Entries.GetSize());
            Assigned.Frames = Placement.Frames;
            Assigned.Atlas  = Placement.Atlas;

            // A frame that is already fired sources the very slice of the atlas it sits in today.
            for (UInt16 Frame = 0; Frame < Placement.Frames; ++Frame)
            {
                Ref<Entry> Record = Destination.Entries.Append();
                Record.Source = Str::Print<"{0}/{1}">(Folder, Atlas->GetKey().GetPath());
                Record.Slice  = Placement.Base + Frame;
                Record.Width  = Atlas->GetWidth();
                Record.Height = Atlas->GetHeight();
            }
        });

        if (!Change.Fired || Change.Source.IsEmpty() || Change.Frames == 0)
        {
            return;
        }

        if (Batches.GetSize() <= Change.Sheet)
        {
            Batches.Resize(Change.Sheet + 1);
        }

        Ref<Batch> Destination = Batches[Change.Sheet];

        Ref<Tileset::Placement> Assigned = Placements.Append();
        Assigned.Motif  = Change.Motif;
        Assigned.Base   = static_cast<UInt16>(Destination.Entries.GetSize());
        Assigned.Frames = Change.Frames;
        Assigned.Atlas  = Change.Sheet;

        // The frames of a run sit side by side on the source, starting at the corner the author named.
        for (UInt16 Frame = 0; Frame < Change.Frames; ++Frame)
        {
            Ref<Entry> Record = Destination.Entries.Append();
            Record.Source = Change.Source;
            Record.X      = static_cast<UInt16>(Change.Origin.GetX()) + Frame * Change.Width;
            Record.Y      = static_cast<UInt16>(Change.Origin.GetY());
            Record.Width  = Change.Width;
            Record.Height = Change.Height;
        }
    }
}
