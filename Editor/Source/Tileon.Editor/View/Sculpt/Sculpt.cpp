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

#include "Sculpt.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sculpt::Sculpt(Ref<Context> Context)
        : Activity    { Context, "Sculpt", true },
          mRepository { Context.GetRepository() },
          mTileset    { Context.GetTileset() },
          mSelection  { 0 }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Sculpt::OnDraw(Ref<UI::Composer> Composer)
    {
        Composer.SetNextWindowSize(1160.0f, 680.0f, ImGuiCond_FirstUseEver);
        Composer.SetNextWindowSizeConstraints(900.0f, 500.0f, 1800.0f, 1400.0f);

        if (Composer.Begin(GetTitle(), mVisible))
        {
            const Real32 Padding = -(ImGui::GetFrameHeightWithSpacing() + 8.0f);

            Composer.BeginChild("##list_panel", ImVec2(180.0f, Padding), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders);
            DrawListPanel(Composer);
            Composer.EndChild();

            if (mRepository.HasTerrain(mSelection))
            {
                Ref<Terrain>        Terrain = mRepository.GetTerrain(mSelection);
                Ref<Tileset::Entry> Entry   = mTileset.GetEntry(Terrain.GetID());

                Composer.SameLine();
                Composer.BeginChild("##left_panel", ImVec2(340.0f, Padding), ImGuiChildFlags_Borders);
                DrawLeftPanel(Composer, Terrain, Entry);
                Composer.EndChild();

                Composer.SameLine();
                Composer.BeginChild("##right_panel", ImVec2(0.0f, Padding), ImGuiChildFlags_Borders);

                if (Entry.Material && Entry.Material->HasCompleted())
                {
                    DrawRightPanel(Composer, Terrain, Entry);
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

    void Sculpt::DrawListPanel(Ref<UI::Composer> Composer)
    {
        const Bool WasPlusClicked = Composer.Button("+", -1.0f);

        if (WasPlusClicked)
        {
            Ref<Terrain> Terrain = mRepository.CreateTerrain();
            mTileset.CreateEntry(Terrain);

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
                    mRepository.DeleteTerrain(mRepository.GetTerrain(Terrain.GetID()));
                }
                Composer.EndPopup();
            }
        });

        Composer.EndChild();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Sculpt::DrawLeftPanel(Ref<UI::Composer> Composer, Ref<Terrain> Terrain, Ref<Tileset::Entry> Entry)
    {
        // Draw the editable fields for the terrain identity properties.
        Composer.Section("Identity");
        Composer.Field<true>("Name", [&]
        {
            Composer.InputText("##name", Terrain.GetName(), [&](ConstStr8 Value)
            {
                Terrain.SetName(Value);
            });
        });

        // Draw the editable fields for the material properties of the tileset entry.
        Composer.Section("Material");
        Composer.Field("Resource", [&]
        {
            Composer.InputTextWithButton("##url", Entry.Path.GetPath(),
                [&](ConstStr8 Value)
                {
                    Ref<Content::Service> Content = GetContext().GetService<Content::Service>();
                    Entry.Path     = Content::Uri(Format("Resources://{}", Value));
                    Entry.Material = Content.Load<Graphic::Material>(Entry.Path);
                },
                "...",
                [&]() { /* TODO: OnBrowseURI */ },
                ImGuiInputTextFlags_EnterReturnsTrue);
        });
        Composer.Field("Span", [&]
        {
            if (Composer.InputIntPair("##span", Entry.Columns, Entry.Rows, "x"))
            {
                Entry.Columns = Max(Entry.Columns, 1);
                Entry.Rows    = Max(Entry.Rows, 1);
            }
        });
        Composer.Field<true>("Color", [&]
        {
            Composer.InputTintSmall("##tint", Entry.Tint);
        });

        // Draw the animation section for the tileset entry.
        Composer.Section("Animation");
        DrawLeftPanelAnimationSection(Composer, Entry);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Sculpt::DrawLeftPanelAnimationSection(Ref<UI::Composer> Composer, Ref<Tileset::Entry> Entry)
    {
        Ref<Animation> Animation = Entry.Animation;

        // Draw the add frame button for the animation, and disable it when the animation is full.
        const Bool WasFull = Animation.IsFull();

        if (WasFull)
        {
            Composer.PushStyleColor(ImGuiCol_Button,        ImVec4(0.30f, 0.30f, 0.30f, 1.0f));
            Composer.PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.30f, 0.30f, 1.0f));
            Composer.PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.30f, 0.30f, 0.30f, 1.0f));
        }

        if (Composer.Button("+ Add Frame", -1.0f) && !WasFull)
        {
            Animation.Insert(Rect(0.0f, 0.0f, 0.0f, 0.0f), 0.1f);
        }

        if (WasFull)
        {
            Composer.PopStyleColor(3);
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
        if (Composer.BeginTable("##anim_table", 7,
            ImGuiTableFlags_BordersOuter  |
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_RowBg         |
            ImGuiTableFlags_SizingStretchSame))
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

            if (Entry.Material)
            {
                ConstTracker<Graphic::Texture> Albedo = Entry.Material->GetTexture(Graphic::TextureSemantic::Albedo);
                TextureWidth  = Albedo ? static_cast<Real32>(Albedo->GetWidth())  : 1.0f;
                TextureHeight = Albedo ? static_cast<Real32>(Albedo->GetHeight()) : 1.0f;
            }

            const auto NormalizeRect = [&](Real32 X, Real32 Y, Real32 Width, Real32 Height)
            {
                return Rect(X / TextureWidth, Y / TextureHeight, (X + Width) / TextureWidth, (Y + Height) / TextureHeight);
            };

            // Draw table entries.
            SInt32 RemoveEntryAt = -1;

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
                }
                Composer.PopItemWidth();

                Composer.TableSetColumnIndex(2);
                Composer.PushItemWidth(-1);
                if (Composer.InputFloat(Base::Format("##ay{}", Keyframe), Y, 0.0f, 0.0f, "%.0f"))
                {
                    Animation.SetFrameData(Keyframe, NormalizeRect(X, Y, W, H));
                }
                Composer.PopItemWidth();

                Composer.TableSetColumnIndex(3);
                Composer.PushItemWidth(-1);
                if (Composer.InputFloat(Base::Format("##aw{}", Keyframe), W, 0.0f, 0.0f, "%.0f"))
                {
                    Animation.SetFrameData(Keyframe, NormalizeRect(X, Y, W, H));
                }
                Composer.PopItemWidth();

                Composer.TableSetColumnIndex(4);
                Composer.PushItemWidth(-1);
                if (Composer.InputFloat(Base::Format("##ah{}", Keyframe), H, 0.0f, 0.0f, "%.0f"))
                {
                    Animation.SetFrameData(Keyframe, NormalizeRect(X, Y, W, H));
                }
                Composer.PopItemWidth();

                Composer.TableSetColumnIndex(5);
                Composer.PushItemWidth(-1);
                if (Composer.InputFloat(Base::Format("##ad{}", Keyframe), Duration, 0.0f, 0.0f, "%.2f"))
                {
                    Animation.SetFrameDuration(Keyframe, Max(Duration, 0.01f));
                }
                Composer.PopItemWidth();

                Composer.TableSetColumnIndex(6);
                if (Composer.SmallButton(Base::Format("x##{}", Keyframe)))
                {
                    RemoveEntryAt = static_cast<SInt32>(Keyframe);
                }
            }

            // Remove the animation frame if the remove button was clicked for any of the entries.
            if (RemoveEntryAt != -1)
            {
                Animation.Remove(static_cast<UInt8>(RemoveEntryAt));
            }

            Composer.EndTable();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Sculpt::DrawRightPanel(Ref<UI::Composer> Composer, Ref<Terrain> Terrain, Ref<Tileset::Entry> Entry)
    {
        if (Composer.BeginTabBar("##right_tabs"))
        {
            for (const Graphic::TextureSemantic Semantic : Enum::Values<Graphic::TextureSemantic>())
            {
                if (ConstTracker<Graphic::Texture> Texture = Entry.Material->GetTexture(Semantic))
                {
                    if (Composer.BeginTabItem(Enum::Name(Semantic)))
                    {
                        mPreviewer.Draw(Composer, Texture);

                        Composer.EndTabItem();
                    }
                }
            }

            // TODO: Show animation for other texture semantics as well, not just albedo.
            if (ConstTracker<Graphic::Texture> Albedo = Entry.Material->GetTexture(Graphic::TextureSemantic::Albedo))
            {
                if (Composer.BeginTabItem("Animation"))
                {
                    const Real32  Density = GetContext().GetDirector().GetDensity();
                    const Vector2 Size(Entry.Columns * Density, Entry.Rows * Density);

                    mPreviewer.Draw(Composer, Albedo, Size, Entry.Animation.GetFrameData(Entry.Keyframe));

                    Composer.EndTabItem();
                }
            }

            Composer.EndTabBar();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Sculpt::DrawBottomBar(Ref<UI::Composer> Composer)
    {
        const Real32     BarHeight = ImGui::GetFrameHeightWithSpacing() + 4.0f;
        constexpr ImVec4 BarColor  = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);

        Composer.PushStyleColor(ImGuiCol_ChildBg, BarColor);
        Composer.BeginChild("##status_bar", ImVec2(0.0f, BarHeight), ImGuiChildFlags_None);
        Composer.PopStyleColor();

        // Vertically center text inside the bar.
        const Real32 PadY = (BarHeight - ImGui::GetTextLineHeight()) * 0.5f - Composer.GetStyle().ItemSpacing.y * 0.5f;
        Composer.SetCursorPosY(PadY);

        if (mRepository.HasTerrain(mSelection))
        {
            ConstRef<Terrain>        Terrain = mRepository.GetTerrain(mSelection);
            ConstRef<Tileset::Entry> Entry   = mTileset.GetEntry(Terrain.GetID());

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

            const UInt32 Status  = Enum::Cast(Entry.Material ? Entry.Material->GetStatus() : Content::Resource::Status::Idle);
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

    void Sculpt::DrawEmptyPanel(Ref<UI::Composer> Composer, ConstStr8 Message)
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