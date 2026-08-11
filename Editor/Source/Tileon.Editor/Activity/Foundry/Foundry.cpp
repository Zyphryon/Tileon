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

#include "Foundry.hpp"
#include "Tileon.Editor/Component/Texture.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Foundry::Foundry(Ref<Context> Context)
        : Activity     { Context, "Foundry" },
          mRepository  { Context.GetRepository() },
          mTileset     { Context.GetTileset() },
          mSelection   { 0 },
          mScroll      { 0 },
          mBrowser     { Context.GetContent(), Toolkit::Browser::Mode::Popup }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Foundry::OnDraw()
    {
        // Adopt a terrain requested from another panel (Palette right-click), scroll it into view, and pull focus.
        if (const SInt64 Request = GetContext().GetInteger("Selection.Tile.Target", 0); Request != 0)
        {
            mSelection = static_cast<UInt16>(Request);
            mScroll    = static_cast<UInt16>(Request);
            mPreviewer.Reset();

            GetContext().SetInteger("Selection.Tile.Target", 0);
            Toolkit::Composer::SetNextWindowFocus();
        }

        Toolkit::Composer::SetNextWindowSize(1160.0f, 680.0f, ImGuiCond_FirstUseEver);
        Toolkit::Composer::SetNextWindowSizeConstraints(900.0f, 500.0f, 1800.0f, 1400.0f);

        if (Toolkit::Composer::Begin(GetTitle(), mVisible))
        {
            const Real32 Padding = -(Toolkit::Composer::GetFrameHeightWithSpacing() + 8.0f);

            Toolkit::Composer::BeginChild("##list_panel", ImVec2(180.0f, Padding), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders);
            DrawListPanel();
            Toolkit::Composer::EndChild();

            if (mRepository.HasTerrain(mSelection))
            {
                Ref<Terrain> Terrain = mRepository.GetTerrain(mSelection);
                Ref<Motif>   Motif   = mTileset.GetMotif(mSelection);

                Toolkit::Composer::SameLine();
                Toolkit::Composer::BeginChild("##left_panel", ImVec2(340.0f, Padding), ImGuiChildFlags_Borders);
                DrawLeftPanel(Terrain, Motif);
                Toolkit::Composer::EndChild();

                Toolkit::Composer::SameLine();
                Toolkit::Composer::BeginChild("##right_panel", ImVec2(0.0f, Padding), ImGuiChildFlags_Borders);

                if (Motif.GetMaterial().IsValid())
                {
                    DrawRightPanel(Motif);
                }
                else
                {
                    DrawEmptyPanel("No material assigned to this terrain");
                }

                Toolkit::Composer::EndChild();
            }
            else
            {
                Toolkit::Composer::SameLine();
                Toolkit::Composer::BeginChild("##empty_panel", ImVec2(0.0f, Padding), ImGuiChildFlags_Borders);
                DrawEmptyPanel("Select a terrain to start editing");
                Toolkit::Composer::EndChild();
            }

            DrawBottomBar();
        }
        Toolkit::Composer::End();

        // Handle the file browser popup for selecting material resources for motifs.
        if (mBrowser.Draw() && !mBrowser.GetSelection().IsEmpty())
        {
            Ref<Motif> Motif = mTileset.GetMotif(mSelection);
            Motif.SetMaterial(Content::Uri(mBrowser.GetSelection()));

            mTileset.Refresh(Motif);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Foundry::DrawListPanel()
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

                    mPreviewer.Reset();
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
                    mPreviewer.Reset();
                }

                Toolkit::Composer::Separator();

                if (Toolkit::Composer::MenuItem("Delete"))
                {
                    mRepository.DeleteTerrain(Terrain.GetID());
                }
                Toolkit::Composer::EndPopup();
            }
        });

        Toolkit::Composer::EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Foundry::DrawLeftPanel(Ref<Terrain> Terrain, Ref<Motif> Motif)
    {
        Bool Dirty = false;

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

        // Draw the editable fields for the material properties of the tileset Motif.
        Toolkit::Composer::Section("Material");

        Toolkit::Composer::Field("Resource");
        Toolkit::Composer::InputTextWithButton("##url", Motif.GetMaterial().GetPath(),
            [&](Text Url)
            {
                if (!Url.IsEmpty())
                {
                    Motif.SetMaterial(Str::Print<"Resources://{0}">(Url));

                    Dirty = true;
                }
            },
            "...",
            [&]
            {
                mBrowser.Open(".mtl");
            },
            ImGuiInputTextFlags_EnterReturnsTrue);
        Toolkit::Composer::Spacing();

        Toolkit::Composer::Field("Span");
        UInt8 Columns = Motif.GetPeriod().GetX();
        UInt8 Rows    = Motif.GetPeriod().GetY();
        if (Toolkit::Composer::InputIntPair("##span", Columns, Rows, "x"))
        {
            Motif.SetPeriod(IntVector2(Columns, Rows));

            Dirty = true;
        }
        Toolkit::Composer::Spacing();

        Toolkit::Composer::Field("Color");
        Toolkit::Composer::PushItemWidth(-1);
        IntColor8 Tint = Motif.GetTint();
        if (Toolkit::Composer::InputTintSmall("##tint", Tint))
        {
            Motif.SetTint(Tint);

            Dirty = true;
        }
        Toolkit::Composer::PopItemWidth();
        Toolkit::Composer::Spacing();

        // Refresh the tileset data for the motif if any of its properties were modified.
        if (Dirty)
        {
            mTileset.Refresh(Motif);
        }

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

    void Foundry::DrawLeftPanelAnimation(Ref<Motif> Motif)
    {
        Animation Animation = Motif.GetAnimation();
        Bool      Dirty     = false;

        // Draw the add frame button for the animation, and disable it when the animation is full.
        if (Toolkit::Composer::DisabledButton("+ Add Frame", Animation.IsFull(), -1.0f))
        {
            Animation.Insert(Rect(0.0f, 0.0f, 0.0f, 0.0f), 0.1f);

            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        // Draw empty hint when there are no frames in the animation.
        if (Animation.IsEmpty())
        {
            constexpr Text kHint = "No Frames";

            Toolkit::Composer::SetCursorPosX((Toolkit::Composer::GetContentRegionAvail().x - Toolkit::Composer::CalcTextSize(kHint).x) * 0.5f);
            Toolkit::Composer::TextDisabled(kHint);
            return;
        }

        // Draw the table of animation frames.
        constexpr ImGuiTableFlags kTableFlags =
            ImGuiTableFlags_BordersOuter  |
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_RowBg         |
            ImGuiTableFlags_SizingStretchSame;
        if (Toolkit::Composer::BeginTable("##anim_table", 7, kTableFlags))
        {
            // Draw table columns and headers.
            Toolkit::Composer::TableSetupColumn("#",    ImGuiTableColumnFlags_WidthFixed,   18.0f);
            Toolkit::Composer::TableSetupColumn("X",    ImGuiTableColumnFlags_WidthStretch);
            Toolkit::Composer::TableSetupColumn("Y",    ImGuiTableColumnFlags_WidthStretch);
            Toolkit::Composer::TableSetupColumn("W",    ImGuiTableColumnFlags_WidthStretch);
            Toolkit::Composer::TableSetupColumn("H",    ImGuiTableColumnFlags_WidthStretch);
            Toolkit::Composer::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthStretch);
            Toolkit::Composer::TableSetupColumn("",     ImGuiTableColumnFlags_WidthFixed,   16.0f);
            Toolkit::Composer::TableHeadersRow();

            // Fetch the sheet to calculate the UV coordinates of the animation frames, which are cut from it.
            Real32 TextureWidth  = 1.0f;
            Real32 TextureHeight = 1.0f;

            if (ConstRetainer<Graphic::Material> Sheet = GetContext().GetForge().GetGallery().GetMaterial(Motif.GetID()))
            {
                ConstRetainer<Graphic::Image> Albedo = Sheet->GetImage(GetTextureID(Texture::Albedo));
                TextureWidth  = Albedo ? static_cast<Real32>(Albedo->GetWidth())  : 1.0f;
                TextureHeight = Albedo ? static_cast<Real32>(Albedo->GetHeight()) : 1.0f;
            }

            const auto NormalizeRect = [&](Real32 X, Real32 Y, Real32 Width, Real32 Height)
            {
                return Rect(X / TextureWidth, Y / TextureHeight, (X + Width) / TextureWidth, (Y + Height) / TextureHeight);
            };

            // Draw table entries.
            SInt32 RemoveMotifAt = -1;

            for (UInt8 Keyframe = 0; Keyframe < Animation.GetCount(); ++Keyframe)
            {
                const Rect Data = Animation.GetFrameData(Keyframe) * Vector2(TextureWidth, TextureHeight);
                Real32 X        = Data.GetMinimumX();
                Real32 Y        = Data.GetMinimumY();
                Real32 W        = Data.GetWidth();
                Real32 H        = Data.GetHeight();
                Real32 Duration = Animation.GetFrameDuration(Keyframe);

                Toolkit::Composer::TableNextRow();

                Toolkit::Composer::TableSetColumnIndex(0);
                Toolkit::Composer::Label("{0}", Keyframe + 1);

                Toolkit::Composer::TableSetColumnIndex(1);
                Toolkit::Composer::PushItemWidth(-1);
                if (Toolkit::Composer::InputFloat(Str32::Print<"##ax{0}">(Keyframe), X, 0.0f, 0.0f, "%.0f"))
                {
                    Animation.SetFrameData(Keyframe, NormalizeRect(X, Y, W, H));

                    Dirty = true;
                }
                Toolkit::Composer::PopItemWidth();

                Toolkit::Composer::TableSetColumnIndex(2);
                Toolkit::Composer::PushItemWidth(-1);
                if (Toolkit::Composer::InputFloat(Str32::Print<"##ay{0}">(Keyframe), Y, 0.0f, 0.0f, "%.0f"))
                {
                    Animation.SetFrameData(Keyframe, NormalizeRect(X, Y, W, H));

                    Dirty = true;
                }
                Toolkit::Composer::PopItemWidth();

                Toolkit::Composer::TableSetColumnIndex(3);
                Toolkit::Composer::PushItemWidth(-1);
                if (Toolkit::Composer::InputFloat(Str32::Print<"##aw{0}">(Keyframe), W, 0.0f, 0.0f, "%.0f"))
                {
                    Animation.SetFrameData(Keyframe, NormalizeRect(X, Y, W, H));

                    Dirty = true;
                }
                Toolkit::Composer::PopItemWidth();

                Toolkit::Composer::TableSetColumnIndex(4);
                Toolkit::Composer::PushItemWidth(-1);
                if (Toolkit::Composer::InputFloat(Str32::Print<"##ah{0}">(Keyframe), H, 0.0f, 0.0f, "%.0f"))
                {
                    Animation.SetFrameData(Keyframe, NormalizeRect(X, Y, W, H));

                    Dirty = true;
                }
                Toolkit::Composer::PopItemWidth();

                Toolkit::Composer::TableSetColumnIndex(5);
                Toolkit::Composer::PushItemWidth(-1);
                if (Toolkit::Composer::InputFloat(Str32::Print<"##ad{0}">(Keyframe), Duration, 0.0f, 0.0f, "%.2f"))
                {
                    Animation.SetFrameDuration(Keyframe, Max(Duration, 0.01f));

                    Dirty = true;
                }
                Toolkit::Composer::PopItemWidth();

                Toolkit::Composer::TableSetColumnIndex(6);
                if (Toolkit::Composer::SmallButton(Str32::Print<"x##{0}">(Keyframe)))
                {
                    RemoveMotifAt = static_cast<SInt32>(Keyframe);
                }
            }

            // Remove the animation frame if the remove button was clicked for any of the entries.
            if (RemoveMotifAt != -1)
            {
                Animation.Remove(static_cast<UInt8>(RemoveMotifAt));

                Dirty = true;
            }

            Toolkit::Composer::EndTable();
        }

        if (Dirty)
        {
            Motif.SetAnimation(Move(Animation));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Foundry::DrawRightPanel(ConstRef<Motif> Motif)
    {
        ConstRef<Tileset::Glyph> Glyph = mTileset.GetGlyph(Motif.GetID());

        if (Toolkit::Composer::BeginTabBar("##right_tabs"))
        {
            // The preview shows the tile as the world draws it, which is one slice of the array it was promoted into.
            if (Glyph.Texture)
            {
                if (Toolkit::Composer::BeginTabItem("Preview"))
                {
                    const Real32  Density = GetContext().GetDirector().GetDensity();
                    const Vector2 Size(Glyph.Period.GetX() * Density, Glyph.Period.GetY() * Density);

                    mPreviewer.Draw(
                        Plugin::ImGuiRenderer::GetTextureID(Glyph.Texture, Glyph.Slice),
                        Size, Rect::One(), Color::FromColor8(Glyph.Tint));

                    Toolkit::Composer::EndTabItem();
                }
            }

            // The remaining tabs show the sheets whole, which is what the frames are cropped out of.
            ConstRetainer<Graphic::Material> Sheet = GetContext().GetForge().GetGallery().GetMaterial(Motif.GetID());

            if (!Sheet)
            {
                Toolkit::Composer::EndTabBar();
                return;
            }

            for (const Texture Semantic : Enum::GetValues<Texture>())
            {
                if (ConstRetainer<Graphic::Image> Texture = Sheet->GetImage(GetTextureID(Semantic)))
                {
                    if (Toolkit::Composer::BeginTabItem(Enum::GetName(Semantic)))
                    {
                        const Vector2 Size(Texture->GetWidth(), Texture->GetHeight());
                        mPreviewer.Draw(Texture->GetHandle(), Size, Rect::One());

                        Toolkit::Composer::EndTabItem();
                    }
                }
            }

            Toolkit::Composer::EndTabBar();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Foundry::DrawBottomBar()
    {
        const Real32 BarHeight = Toolkit::Composer::GetFrameHeightWithSpacing() + 4.0f;

        Toolkit::Composer::PushStyleColor(ImGuiCol_ChildBg, Toolkit::Composer::GetStyleColorVec4(ImGuiCol_MenuBarBg));
        Toolkit::Composer::BeginChild("##status_bar", ImVec2(0.0f, BarHeight), ImGuiChildFlags_None);
        Toolkit::Composer::PopStyleColor();

        // Vertically center text inside the bar.
        const Real32 PadY = (BarHeight - Toolkit::Composer::GetTextLineHeight()) * 0.5f - Toolkit::Composer::GetStyle().ItemSpacing.y * 0.5f;
        Toolkit::Composer::SetCursorPosY(PadY);

        if (mRepository.HasTerrain(mSelection))
        {
            ConstRef<Terrain>                Terrain = mRepository.GetTerrain(mSelection);
            ConstRef<Motif>                  Motif   = mTileset.GetMotif(Terrain.GetID());
            ConstRetainer<Graphic::Material> Sheet   = GetContext().GetForge().GetGallery().GetMaterial(Motif.GetID());

            Toolkit::Composer::SetCursorPosX(Toolkit::Composer::GetStyle().ItemSpacing.x);
            Toolkit::Composer::Label("{0:04}  {1}", Terrain.GetID(), Terrain.GetName().IsEmpty() ? "(Unnamed)" : Terrain.GetName());

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

            const UInt32 Status  = Enum::Cast(Sheet ? Sheet->GetStatus() : Content::Resource::Status::Idle);
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

        Toolkit::Composer::EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Foundry::DrawEmptyPanel(Text Message)
    {
        constexpr Text kIcon = "?";

        const ImVec2 Available = Toolkit::Composer::GetContentRegionAvail();
        const ImVec2 IconSize  = Toolkit::Composer::CalcTextSize(kIcon);
        const ImVec2 HintSize  = Toolkit::Composer::CalcTextSize(Message);

        Toolkit::Composer::SetCursorPosX((Available.x - IconSize.x) * 0.5f);
        Toolkit::Composer::SetCursorPosY((Available.y - IconSize.y + 8.0f + HintSize.y) * 0.5f);
        Toolkit::Composer::TextDisabled(kIcon);

        Toolkit::Composer::SetCursorPosX((Available.x - HintSize.x) * 0.5f);
        Toolkit::Composer::TextDisabled(Message);
    }
}