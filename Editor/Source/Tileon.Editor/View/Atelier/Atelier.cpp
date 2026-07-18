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

#include "Atelier.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Atelier::Atelier(Ref<Context> Context)
        : Activity  { Context, kTitle, true  },
          mWorkshop { Context },
          mGizmo    { Context },
          mTarget   { Renderer::Target::Albedo }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::OnDraw(Ref<UI::Composer> Composer)
    {
        mWorkshop.Tick();

        Composer.SetNextWindowPos(Composer.GetViewportCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
        Composer.SetNextWindowSize(800.0f, 600.0f, ImGuiCond_FirstUseEver);
        Composer.SetNextWindowSizeConstraints(320.0f, 200.0f);

        if (Composer.Begin(GetTitle(), mVisible))
        {
            Composer.BeginChild("##mainbar", ImVec2(0.0f, 40.0f), ImGuiChildFlags_Borders);
            DrawToolbar(Composer);
            Composer.EndChild();

            Composer.PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            Composer.BeginChild("##viewport", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
            Composer.PopStyleColor();

            DrawViewport(Composer);
            Composer.EndChild();
        }
        else
        {
            mWorkshop.ClearPreview();
        }
        Composer.End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawToolbar(Ref<UI::Composer> Composer)
    {
        // Draw mode switch button to toggle between tile editing and entity editing modes.
        switch (mWorkshop.GetMode())
        {
        case Workshop::Mode::Tile:
            if (Composer.Button(ICON_FA_MAP "##mode", 32.0f))
            {
                mWorkshop.SetBrush(Workshop::Brush::Pencil);
                mWorkshop.SetMode(Workshop::Mode::Entity);
            }
            break;
        case Workshop::Mode::Entity:
            if (Composer.Button(ICON_FA_CUBE "##mode", 32.0f))
            {
                mWorkshop.SetBrush(Workshop::Brush::Pencil);
                mWorkshop.SetMode(Workshop::Mode::Tile);
            }
            break;
        }
        Composer.SameLine();


        // Draw common toolbar elements that are always visible.
        if (Composer.Button(ICON_FA_MAGNIFYING_GLASS_PLUS "##zoom_in", 32.0f))
        {
            const Real32 Current = GetContext().GetDirector().GetZoom();
            GetContext().GetDirector().SetZoom(Current * 0.9f);
        }
        Composer.SameLine();

        if (Composer.Button(ICON_FA_MAGNIFYING_GLASS_MINUS "##zoom_out", 32.0f))
        {
            const Real32 Current = GetContext().GetDirector().GetZoom();
            GetContext().GetDirector().SetZoom(Current * 1.1f);
        }
        Composer.SameLine();

        if (Composer.Button(ICON_FA_HOUSE "##reset_view", 32.0f))
        {
            GetContext().GetDirector().SetZoom(1.0f);
        }
        Composer.SameLine();

        // Camera position read-out that doubles as a "go to": shows the view centre in absolute world tiles and
        // re centres the camera when a new coordinate is committed (Enter).
        {
            Ref<Director>   Director = GetContext().GetDirector();
            const Placement Centre   = Director.GetPosition();

            Real32 CameraX = static_cast<Real32>(Centre.GetAbsoluteX());
            Real32 CameraY = static_cast<Real32>(Centre.GetAbsoluteY());

            Bool GoTo = false;

            Composer.Label("X");
            Composer.SameLine();
            Composer.SetNextItemWidth(72.0f);
            GoTo |= Composer.InputFloat("##camera_x", CameraX, 0.0f, 0.0f, "%.1f", ImGuiInputTextFlags_EnterReturnsTrue);
            Composer.SameLine();

            Composer.Label("Y");
            Composer.SameLine();
            Composer.SetNextItemWidth(72.0f);
            GoTo |= Composer.InputFloat("##camera_y", CameraY, 0.0f, 0.0f, "%.1f", ImGuiInputTextFlags_EnterReturnsTrue);
            Composer.SameLine();

            if (GoTo)
            {
                Director.SetPosition(Placement::FromAbsolute(CameraX, CameraY));
            }
        }

        Composer.SeparatorEx(ImGuiSeparatorFlags_Vertical);
        Composer.SameLine();

        switch (mWorkshop.GetMode())
        {
        case Workshop::Mode::Tile:
            DrawTileToolbar(Composer);
            break;
        case Workshop::Mode::Entity:
            DrawEntityToolbar(Composer);
            break;
        }
        Composer.SameLine();

        Composer.SeparatorEx(ImGuiSeparatorFlags_Vertical);
        Composer.SameLine();

        // Draw the diagnostic overlay toggles.
        DrawDebugButton(Composer, Renderer::Debug::Grid, ICON_FA_BORDER_ALL);
        Composer.SameLine();

        DrawDebugButton(Composer, Renderer::Debug::Boundaries, ICON_FA_OBJECT_GROUP);
        Composer.SameLine();

        // Draw projection mode and frame selector combos on the far right of the toolbar.
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

        Composer.SetCursorPosX(Composer.GetWindowWidth() - 200.0f);
        Composer.SetNextItemWidth(96.0f);

        const Director::Mode CurrentMode = GetContext().GetDirector().GetMode();

        if (Composer.BeginCombo("##mode", Enum::GetName(CurrentMode)))
        {
            for (const Director::Mode Type : Enum::GetValues<Director::Mode>())
            {
                const Bool Selected = (CurrentMode == Type);

                if (Composer.Selectable(Enum::GetName(Type), Selected))
                {
                    GetContext().GetDirector().SetMode(Type);
                }

                if (Selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            Composer.EndCombo();
        }

        Composer.SameLine();
        Composer.SetNextItemWidth(96.0f);

        if (Composer.BeginCombo("##target", Enum::GetName(mTarget)))
        {
            for (const Renderer::Target Type : Enum::GetValues<Renderer::Target>())
            {
                const Bool Selected = (mTarget == Type);

                if (Composer.Selectable(Enum::GetName(Type), Selected))
                {
                    mTarget = Type;
                }

                if (Selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            Composer.EndCombo();
        }

        ImGui::PopStyleVar();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawBrushButton(Ref<UI::Composer> Composer, Workshop::Brush Brush, Text Icon)
    {
        const Bool Active = (mWorkshop.GetBrush() == Brush);

        if (Active)
        {
            Composer.PushStyleColor(ImGuiCol_Button,        Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
            Composer.PushStyleColor(ImGuiCol_ButtonHovered, Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        if (Composer.Button(String<64>::Print<"{0}##{1}">(Icon, Enum::GetName(Brush)), 32.0f))
        {
            mWorkshop.SetBrush(Brush);
        }

        if (Active)
        {
            Composer.PopStyleColor(2);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawDebugButton(Ref<UI::Composer> Composer, Renderer::Debug Overlay, Text Icon)
    {
        const Bool Active = GetContext().GetRenderer().HasProperty(Overlay);

        if (Active)
        {
            Composer.PushStyleColor(ImGuiCol_Button,        Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
            Composer.PushStyleColor(ImGuiCol_ButtonHovered, Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        if (Composer.Button(String<64>::Print<"{0}##{1}">(Icon, Enum::GetName(Overlay)), 32.0f))
        {
            GetContext().GetRenderer().SetProperty(Overlay, !Active);
        }

        if (Active)
        {
            Composer.PopStyleColor(2);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawTileToolbar(Ref<UI::Composer> Composer)
    {
        DrawBrushButton(Composer, Workshop::Brush::Hand,   ICON_FA_HAND);
        Composer.SameLine();

        DrawBrushButton(Composer, Workshop::Brush::Select, ICON_FA_ARROW_POINTER);
        Composer.SameLine();

        DrawBrushButton(Composer, Workshop::Brush::Pencil, ICON_FA_BRUSH);
        Composer.SameLine();

        DrawBrushButton(Composer, Workshop::Brush::Bucket, ICON_FA_FILL_DRIP);
        Composer.SameLine();

        Composer.SeparatorEx(ImGuiSeparatorFlags_Vertical);
        Composer.SameLine();

        // Draw the layer selection buttons for switching between layers.
        const auto DrawLayerButton = [&](Workshop::Level Level, Text Icon)
        {
            const Bool Active = (mWorkshop.GetLevel() == Level);

            if (Active)
            {
                Composer.PushStyleColor(ImGuiCol_Button,        Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
                Composer.PushStyleColor(ImGuiCol_ButtonHovered, Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
            }

            if (Composer.Button(String<64>::Print<"{0}##{1}">(Icon, Enum::GetName(Level)), 32.0f))
            {
                mWorkshop.SetLevel(Level);
            }

            if (Active)
            {
                Composer.PopStyleColor(2);
            }
        };

        ImGui::AlignTextToFramePadding();
        ImGui::Text(ICON_FA_LAYER_GROUP);
        Composer.SameLine();

        DrawLayerButton(Workshop::Level::Base,   ICON_FA_1);
        Composer.SameLine();

        DrawLayerButton(Workshop::Level::Detail, ICON_FA_2);
        Composer.SameLine();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawEntityToolbar(Ref<UI::Composer> Composer)
    {
        // Entities are placed one at a time, so the area-filling bucket has no meaning here.
        DrawBrushButton(Composer, Workshop::Brush::Hand,   ICON_FA_HAND);
        Composer.SameLine();

        DrawBrushButton(Composer, Workshop::Brush::Select, ICON_FA_ARROW_POINTER);
        Composer.SameLine();

        DrawBrushButton(Composer, Workshop::Brush::Pencil, ICON_FA_BRUSH);
        Composer.SameLine();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawViewport(Ref<UI::Composer> Composer)
    {
        Ref<Renderer> Renderer = GetContext().GetRenderer();
        Ref<Director> Director = GetContext().GetDirector();

        // Draw the current frame rendered by the renderer to the interface.
        const Graphic::Object Texture = Renderer.GetTarget(mTarget);

        if (GetContext().GetGraphic().GetDescription().Capabilities.IsOriginBottomLeft)
        {
            Composer.Image(Texture, Composer.GetContentRegionAvail(), ImVec4(0, 1, 1, 0));
        }
        else
        {
            Composer.Image(Texture, Composer.GetContentRegionAvail(), ImVec4(0, 0, 1, 1));
        }

        const ImVec2 ViewportOrigin = Composer.GetItemRectMin();
        const ImVec2 ViewportSize   = Composer.GetItemRectSize();

        // Mark the selected entity with corner brackets, drawn as an overlay so it shows under every brush.
        if (const UInt64 Selected = GetContext().GetInteger("Selection.Entity", 0))
        {
            const Scene::Entity Actor = GetContext().GetScene().GetEntity(Selected);

            if (Actor.IsValid())
            {
                if (Actor.Has<Tileon::Bound>())
                {
                    const IntRect AABB   = Actor.Get<Tileon::Bound>().GetRect();
                    const Real32  RangeX = Director.GetViewport().GetX() * Director.GetDensity();
                    const Real32  RangeY = Director.GetViewport().GetY() * Director.GetDensity();

                    const auto ToScreen = [&](Real32 X, Real32 Y)
                    {
                        const Vector2 Pixel = Director.GetScreenCoordinates(Placement(0, 0, X, Y));
                        return ImVec2(
                            ViewportOrigin.x + (Pixel.GetX() / RangeX) * ViewportSize.x,
                            ViewportOrigin.y + (Pixel.GetY() / RangeY) * ViewportSize.y);
                    };

                    // The world is unrotated, so two opposite corners bound an axis-aligned screen rect.
                    const ImVec2 A = ToScreen(AABB.GetMinimumX(), AABB.GetMinimumY());
                    const ImVec2 B = ToScreen(AABB.GetMaximumX(), AABB.GetMaximumY());

                    const Real32 MinX = Min(A.x, B.x);
                    const Real32 MinY = Min(A.y, B.y);
                    const Real32 MaxX = Max(A.x, B.x);
                    const Real32 MaxY = Max(A.y, B.y);

                    constexpr UInt32 Color = IM_COL32(255, 170, 40, 235);
                    constexpr Real32 Thick = 2.0f;
                    const Real32     Arm   = Min(24.0f, Min(MaxX - MinX, MaxY - MinY) * 0.35f);

                    const Ptr<ImDrawList> List = ImGui::GetWindowDrawList();
                    List->AddLine(ImVec2(MinX, MinY), ImVec2(MinX + Arm, MinY), Color, Thick);
                    List->AddLine(ImVec2(MinX, MinY), ImVec2(MinX, MinY + Arm), Color, Thick);
                    List->AddLine(ImVec2(MaxX, MinY), ImVec2(MaxX - Arm, MinY), Color, Thick);
                    List->AddLine(ImVec2(MaxX, MinY), ImVec2(MaxX, MinY + Arm), Color, Thick);
                    List->AddLine(ImVec2(MaxX, MaxY), ImVec2(MaxX - Arm, MaxY), Color, Thick);
                    List->AddLine(ImVec2(MaxX, MaxY), ImVec2(MaxX, MaxY - Arm), Color, Thick);
                    List->AddLine(ImVec2(MinX, MaxY), ImVec2(MinX + Arm, MaxY), Color, Thick);
                    List->AddLine(ImVec2(MinX, MaxY), ImVec2(MinX, MaxY - Arm), Color, Thick);
                }

                if (ImGui::IsWindowFocused())
                {
                    if (Composer.IsKeyPressed(ImGuiKey_Delete))
                    {
                        Actor.Add<Dispose>();
                        Actor.Add<Persist>();
                    }
                }
            }
        }

        Bool Manipulating = false;

        if (mWorkshop.GetBrush() == Workshop::Brush::Select)
        {
            if (ImGui::IsWindowFocused())
            {
                if (Composer.IsKeyPressed(ImGuiKey_Q))
                {
                    mGizmo.SetMode(Gizmo::Mode::Move);
                }
                else if (Composer.IsKeyPressed(ImGuiKey_W))
                {
                    mGizmo.SetMode(Gizmo::Mode::Rotate);
                }
                else if (Composer.IsKeyPressed(ImGuiKey_E))
                {
                    mGizmo.SetMode(Gizmo::Mode::Scale);
                }
            }

            const UInt64        Selection = GetContext().GetInteger("Selection.Entity", 0);
            const Scene::Entity Actor     = GetContext().GetScene().GetEntity(Selection);

            Manipulating = mGizmo.Draw(Composer, Actor, ViewportOrigin, ViewportSize);
        }

        // Handle interactions with the viewport, such as hovering and clicking to manipulate the scene.
        // A drag that belongs to the handles must not also reach the brush, or picking would fight the gizmo.
        if (!Manipulating && ImGui::IsItemHovered())
        {
            const UInt32 AbsoluteX  = Composer.GetMousePos().x - Composer.GetItemRectMin().x;
            const Real32 NormalizeX = AbsoluteX / Composer.GetItemRectSize().x;
            const UInt32 AbsoluteY  = Composer.GetMousePos().y - Composer.GetItemRectMin().y;
            const Real32 NormalizeY = AbsoluteY / Composer.GetItemRectSize().y;

            // Ctrl + wheel resizes the pending entity.
            if (const Real32 Wheel = Composer.GetMouseWheel(); Wheel != 0.0f)
            {
                if (mWorkshop.HasPreview() && ImGui::GetIO().KeyCtrl)
                {
                    mWorkshop.AdjustPreviewScale(Wheel);
                }
                else
                {
                    constexpr Real32 kFactor = 1.25f;

                    const Real32  Step = -Wheel;
                    const Real32  Zoom = Director.GetZoom() * (Step > 0.0f ? kFactor : (1.0f / kFactor));
                    const Vector2 Pixel(
                        NormalizeX * Director.GetViewport().GetX() * Director.GetDensity(),
                        NormalizeY * Director.GetViewport().GetY() * Director.GetDensity());
                    Director.Focus(Director.GetWorldCoordinates(Pixel), Zoom);
                }
            }

            // Hold Q / E to rotate the pending entity smoothly; hold Shift for fine control.
            if (mWorkshop.HasPreview())
            {
                const Real32 Direction =
                    (ImGui::IsKeyDown(ImGuiKey_E) ? 1.0f : 0.0f) -
                    (ImGui::IsKeyDown(ImGuiKey_Q) ? 1.0f : 0.0f);

                if (Direction != 0.0f)
                {
                    const Real32 Speed = ImGui::GetIO().KeyShift ? 30.0f : 120.0f;
                    mWorkshop.AdjustPreviewRotation(Angle::FromDegrees(Direction * Speed * ImGui::GetIO().DeltaTime));
                }
            }

            // Handle mouse dragging for panning the camera when the hand brush is selected.
            if (mWorkshop.GetBrush() == Workshop::Brush::Hand)
            {
                mWorkshop.ClearPreview();

                if (Composer.IsMouseDragging(ImGuiMouseButton_Left))
                {
                    const ImVec2 Delta = Composer.GetMouseDelta();
                    const ImVec2 Size  = Composer.GetItemRectSize();

                    const Real32 ScaleX = Director.GetViewport().GetX() * Director.GetDensity() / Size.x;
                    const Real32 ScaleY = Director.GetViewport().GetY() * Director.GetDensity() / Size.y;

                    const Vector2 OldPosition(AbsoluteX * ScaleX, AbsoluteY* ScaleY);
                    const Vector2 NewPosition((AbsoluteX - Delta.x) * ScaleX, (AbsoluteY - Delta.y) * ScaleY);

                    const Placement OldPlacement = Director.GetWorldCoordinates(OldPosition);
                    const Placement NewPlacement = Director.GetWorldCoordinates(NewPosition);
                    Director.SetPosition(Placement::Normalize(Director.GetPosition() + NewPlacement - OldPlacement));
                }
            }
            else
            {
                const Bool IsLeftButton  = Composer.IsMouseClicked(ImGuiMouseButton_Left);
                const Bool IsRightButton = Composer.IsMouseClicked(ImGuiMouseButton_Right);

                // Get the currently selected object from the context, which depends on the active editing mode.
                const UInt32 Selection  = GetContext().GetInteger(
                    (mWorkshop.GetMode() == Workshop::Mode::Tile)
                        ? "Selection.Tile"_Text
                        : "Selection.Archetype"_Text, 0);

                // The select brush picks whatever sits under the cursor, so it carries no object from the palette.
                const Bool IsPicking = (mWorkshop.GetBrush() == Workshop::Brush::Select);

                const Placement Cursor = Director.GetWorldCoordinates(Vector2(AbsoluteX, AbsoluteY));

                // Show the pending entity under the cursor before it is committed by a click.
                mWorkshop.UpdatePreview(Cursor, Selection);

                // Handle left-click for adding and right-click for removing tiles.
                if (IsRightButton || (IsLeftButton && (IsPicking || Selection != 0)))
                {
                    const Workshop::Command Command = IsLeftButton
                        ? Workshop::Command::Add
                        : Workshop::Command::Remove;
                    mWorkshop.Execute(Command, Cursor, Selection);
                }
            }
        }
        else
        {
            mWorkshop.ClearPreview();
        }
    }
}