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

#include "Atlas.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Atlas::Atlas(Ref<Context> Context)
        : Activity    { Context, "Atlas" },
          mRepository { Context.GetRepository() },
          mTileset    { Context.GetTileset() },
          mSelection  { 0 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atlas::OnDraw(Ref<UI::Composer> Composer)
    {
        Composer.SetNextWindowSize(1160.0f, 680.0f, ImGuiCond_FirstUseEver);
        Composer.SetNextWindowSizeConstraints(900.0f, 500.0f, 1800.0f, 1400.0f);

        if (Composer.Begin(GetTitle(), mVisible))
        {
            const Real32 Padding = -(Composer.GetFrameHeightWithSpacing() + 8.0f);

            Composer.BeginChild("##list_panel", ImVec2(180.0f, Padding), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders);
            DrawListPanel(Composer);
            Composer.EndChild();

            if (mRepository.HasTerrain(mSelection))
            {
                Ref<Terrain> Terrain = mRepository.GetTerrain(mSelection);
                Ref<Motif>   Motif   = mTileset.GetMotif(mSelection);

                Composer.SameLine();
                Composer.BeginChild("##left_panel", ImVec2(340.0f, Padding), ImGuiChildFlags_Borders);
                DrawLeftPanel(Composer, Terrain, Motif);
                Composer.EndChild();

                Composer.SameLine();
                Composer.BeginChild("##right_panel", ImVec2(0.0f, Padding), ImGuiChildFlags_Borders);

                if (Motif.GetMaterial().IsValid())
                {
                    DrawRightPanel(Composer, mTileset.GetGlyph(Motif.GetID()));
                }
                else
                {
                    DrawEmptyPanel(Composer, "No material assigned to this terrain");
                }

                Composer.EndChild();
            }
            else
            {
                Composer.SameLine();
                Composer.BeginChild("##empty_panel", ImVec2(0.0f, Padding), ImGuiChildFlags_Borders);
                DrawEmptyPanel(Composer, "Select a terrain to start editing");
                Composer.EndChild();
            }

            DrawBottomBar(Composer);
        }
        Composer.End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atlas::DrawListPanel(Ref<UI::Composer> Composer)
    {
        const Bool WasPlusClicked = Composer.Button("+", -1.0f);

        if (WasPlusClicked)
        {
            Ref<Terrain> Terrain = mRepository.CreateTerrain();

            mSelection = Terrain.GetID();
        }

        Composer.Separator();
        Composer.BeginChild("##list_scroll", ImVec2(0.0f, 0.0f));

        mRepository.ForEachTerrain([&](ConstRef<Terrain> Terrain)
        {
            const Bool Selected = (mSelection == Terrain.GetID());

            Composer.Selectable(Base::Format("{:04} {}", Terrain.GetID(), Terrain.GetName()), Selected);

            if (Composer.IsItemClicked())
            {
                if (mSelection != Terrain.GetID())
                {
                    mSelection = Terrain.GetID();

                    mPreviewer.Reset();
                }
            }

            if (Selected && WasPlusClicked)
            {
                Composer.SetScrollHereY(0.5f);
            }

            if (Composer.BeginPopupContextItem())
            {
                if (Composer.MenuItem("Delete"))
                {
                    mRepository.DeleteTerrain(Terrain.GetID());
                }
                Composer.EndPopup();
            }
        });

        Composer.EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atlas::DrawLeftPanel(Ref<UI::Composer> Composer, Ref<Terrain> Terrain, Ref<Motif> Motif)
    {
        Bool Dirty = false;

        // Draw the editable fields for the terrain identity properties.
        Composer.Section("Identity");

        Composer.Field("Name");
        Composer.PushItemWidth(-1);
        Composer.InputText("##name", Terrain.GetName(), [&](ConstStr8 Value)
        {
            Terrain.SetName(Value);
        });
        Composer.PopItemWidth();
        Composer.Spacing();

        // Draw the editable fields for the material properties of the tileset Motif.
        Composer.Section("Material");

        Composer.Field("Resource");
        Composer.InputTextWithButton("##url", Motif.GetMaterial().GetPath(),
            [&](ConstStr8 Url)
            {
                if (!Url.empty())
                {
                    Motif.SetMaterial(Format("Resources://{}", Url));

                    Dirty = true;
                }
            },
            "...",
            [&]()
            {
                /* TODO: OnBrowseURI */
            },
            ImGuiInputTextFlags_EnterReturnsTrue);
        Composer.Spacing();

        Composer.Field("Span");
        UInt8 Columns = Motif.GetSpan().GetX();
        UInt8 Rows    = Motif.GetSpan().GetY();
        if (Composer.InputIntPair("##span", Columns, Rows, "x"))
        {
            Motif.SetSpan(IntVector2(Columns, Rows));

            Dirty = true;
        }
        Composer.Spacing();

        Composer.Field("Color");
        Composer.PushItemWidth(-1);
        IntColor8 Tint = Motif.GetTint();
        if (Composer.InputTintSmall("##tint", Tint))
        {
            Motif.SetTint(Tint);

            Dirty = true;
        }
        Composer.PopItemWidth();
        Composer.Spacing();

        // Refresh the tileset data for the motif if any of its properties were modified.
        if (Dirty)
        {
            mTileset.Refresh(Motif);
        }

        // Draw the animation section for the tileset Motif.
        Composer.Section("Animation");
        DrawLeftPanelAnimation(Composer, Motif, mTileset.GetGlyph(Motif.GetID()));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atlas::DrawLeftPanelAnimation(Ref<UI::Composer> Composer, Ref<Motif> Motif, ConstRef<Tileset::Glyph> Glyph)
    {
        Animation Animation = Motif.GetAnimation();
        Bool      Dirty     = false;

        // Draw the add frame button for the animation, and disable it when the animation is full.
        if (Composer.DisabledButton("+ Add Frame", Animation.IsFull(), -1.0f))
        {
            Animation.Insert(Rect(0.0f, 0.0f, 0.0f, 0.0f), 0.1f);

            Dirty = true;
        }

        Composer.Spacing();

        // Draw empty hint when there are no frames in the animation.
        if (Animation.IsEmpty())
        {
            constexpr ConstStr8 kHint = "No Frames";

            Composer.SetCursorPosX((Composer.GetContentRegionAvail().x - Composer.CalcTextSize(kHint).x) * 0.5f);
            Composer.TextDisabled(kHint);
            return;
        }

        // Draw the table of animation frames.
        constexpr ImGuiTableFlags kTableFlags =
            ImGuiTableFlags_BordersOuter  |
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_RowBg         |
            ImGuiTableFlags_SizingStretchSame;
        if (Composer.BeginTable("##anim_table", 7, kTableFlags))
        {
            // Draw table columns and headers.
            Composer.TableSetupColumn("#",    ImGuiTableColumnFlags_WidthFixed,   18.0f);
            Composer.TableSetupColumn("X",    ImGuiTableColumnFlags_WidthStretch);
            Composer.TableSetupColumn("Y",    ImGuiTableColumnFlags_WidthStretch);
            Composer.TableSetupColumn("W",    ImGuiTableColumnFlags_WidthStretch);
            Composer.TableSetupColumn("H",    ImGuiTableColumnFlags_WidthStretch);
            Composer.TableSetupColumn("Time", ImGuiTableColumnFlags_WidthStretch);
            Composer.TableSetupColumn("",     ImGuiTableColumnFlags_WidthFixed,   16.0f);
            Composer.TableHeadersRow();

            // Fetch the texture to calculate the UV coordinates of the animation frames.
            Real32 TextureWidth  = 1.0f;
            Real32 TextureHeight = 1.0f;

            if (Glyph.Material)
            {
                ConstTracker<Graphic::Texture> Albedo = Glyph.Material->GetTexture(Graphic::TextureSemantic::Albedo);
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

                Composer.TableNextRow();

                Composer.TableSetColumnIndex(0);
                Composer.Label("{}", Keyframe);

                Composer.TableSetColumnIndex(1);
                Composer.PushItemWidth(-1);
                if (Composer.InputFloat(Base::Format("##ax{}", Keyframe), X, 0.0f, 0.0f, "%.0f"))
                {
                    Animation.SetFrameData(Keyframe, NormalizeRect(X, Y, W, H));

                    Dirty = true;
                }
                Composer.PopItemWidth();

                Composer.TableSetColumnIndex(2);
                Composer.PushItemWidth(-1);
                if (Composer.InputFloat(Base::Format("##ay{}", Keyframe), Y, 0.0f, 0.0f, "%.0f"))
                {
                    Animation.SetFrameData(Keyframe, NormalizeRect(X, Y, W, H));

                    Dirty = true;
                }
                Composer.PopItemWidth();

                Composer.TableSetColumnIndex(3);
                Composer.PushItemWidth(-1);
                if (Composer.InputFloat(Base::Format("##aw{}", Keyframe), W, 0.0f, 0.0f, "%.0f"))
                {
                    Animation.SetFrameData(Keyframe, NormalizeRect(X, Y, W, H));

                    Dirty = true;
                }
                Composer.PopItemWidth();

                Composer.TableSetColumnIndex(4);
                Composer.PushItemWidth(-1);
                if (Composer.InputFloat(Base::Format("##ah{}", Keyframe), H, 0.0f, 0.0f, "%.0f"))
                {
                    Animation.SetFrameData(Keyframe, NormalizeRect(X, Y, W, H));

                    Dirty = true;
                }
                Composer.PopItemWidth();

                Composer.TableSetColumnIndex(5);
                Composer.PushItemWidth(-1);
                if (Composer.InputFloat(Base::Format("##ad{}", Keyframe), Duration, 0.0f, 0.0f, "%.2f"))
                {
                    Animation.SetFrameDuration(Keyframe, Max(Duration, 0.01f));

                    Dirty = true;
                }
                Composer.PopItemWidth();

                Composer.TableSetColumnIndex(6);
                if (Composer.SmallButton(Base::Format("x##{}", Keyframe)))
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

            Composer.EndTable();
        }

        if (Dirty)
        {
            Motif.SetAnimation(Move(Animation));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atlas::DrawRightPanel(Ref<UI::Composer> Composer, ConstRef<Tileset::Glyph> Glyph)
    {
        if (Composer.BeginTabBar("##right_tabs"))
        {
            for (const Graphic::TextureSemantic Semantic : Enum::Values<Graphic::TextureSemantic>())
            {
                if (ConstTracker<Graphic::Texture> Texture = Glyph.Material->GetTexture(Semantic))
                {
                    if (Composer.BeginTabItem(Enum::Name(Semantic)))
                    {
                        const Vector2 Size(Texture->GetWidth(), Texture->GetHeight());
                        mPreviewer.Draw(Composer, Texture->GetID(), Size, Rect::One());

                        Composer.EndTabItem();
                    }
                }
            }

            // TODO: Show animation for other texture semantics as well, not just albedo.
            if (ConstTracker<Graphic::Texture> Albedo = Glyph.Material->GetTexture(Graphic::TextureSemantic::Albedo))
            {
                if (Composer.BeginTabItem("Animation"))
                {
                    const Real32  Density = GetContext().GetDirector().GetDensity();
                    const Vector2 Size(Glyph.Span.GetX() * Density, Glyph.Span.GetY() * Density);

                    mPreviewer.Draw(Composer, Albedo->GetID(), Size, Glyph.Crop, Color::FromColor8(Glyph.Tint));

                    Composer.EndTabItem();
                }
            }

            Composer.EndTabBar();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atlas::DrawBottomBar(Ref<UI::Composer> Composer)
    {
        const Real32 BarHeight = Composer.GetFrameHeightWithSpacing() + 4.0f;

        Composer.PushStyleColor(ImGuiCol_ChildBg, Composer.GetStyleColorVec4(ImGuiCol_MenuBarBg));
        Composer.BeginChild("##status_bar", ImVec2(0.0f, BarHeight), ImGuiChildFlags_None);
        Composer.PopStyleColor();

        // Vertically center text inside the bar.
        const Real32 PadY = (BarHeight - Composer.GetTextLineHeight()) * 0.5f - Composer.GetStyle().ItemSpacing.y * 0.5f;
        Composer.SetCursorPosY(PadY);

        if (mRepository.HasTerrain(mSelection))
        {
            ConstRef<Terrain>        Terrain = mRepository.GetTerrain(mSelection);
            ConstRef<Motif>          Motif   = mTileset.GetMotif(Terrain.GetID());
            ConstRef<Tileset::Glyph> Glyph   = mTileset.GetGlyph(Motif.GetID());

            Composer.SetCursorPosX(Composer.GetStyle().ItemSpacing.x);
            Composer.Label("{:04}  {}", Terrain.GetID(), Terrain.GetName().empty() ? "(Unnamed)" : Terrain.GetName());

            constexpr ConstStr8 kStatusLabel[] = {
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

            const UInt32 Status  = Enum::Cast(Glyph.Material ? Glyph.Material->GetStatus() : Content::Resource::Status::Idle);
            const Real32 StatusW = Composer.CalcTextSize(kStatusLabel[Status]).x + Composer.GetStyle().ItemSpacing.x * 2.0f;

            Composer.SameLine(Composer.GetWindowWidth() - StatusW);
            Composer.SetCursorPosY(PadY);
            Composer.TextColored(kStatusColor[Status], kStatusLabel[Status]);
        }
        else
        {
            constexpr ConstStr8 Hint = "No terrain selected";

            Composer.SetCursorPosX((Composer.GetWindowWidth() - Composer.CalcTextSize(Hint).x) * 0.5f);
            Composer.TextDisabled(Hint);
        }

        Composer.EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atlas::DrawEmptyPanel(Ref<UI::Composer> Composer, ConstStr8 Message)
    {
        constexpr ConstStr8 kIcon = "?";

        const ImVec2 Available = Composer.GetContentRegionAvail();
        const ImVec2 IconSize  = Composer.CalcTextSize(kIcon);
        const ImVec2 HintSize  = Composer.CalcTextSize(Message);

        Composer.SetCursorPosX((Available.x - IconSize.x) * 0.5f);
        Composer.SetCursorPosY((Available.y - IconSize.y + 8.0f + HintSize.y) * 0.5f);
        Composer.TextDisabled(kIcon);

        Composer.SetCursorPosX((Available.x - HintSize.x) * 0.5f);
        Composer.TextDisabled(Message);
    }
}