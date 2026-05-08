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

#include "Scene.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Scene(Ref<Context> Context)
        : Activity  { Context, kTitle, true  },
          mWorkshop { Context.GetController() },
          mFrame    { Renderer::Frame::Albedo }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scene::OnDraw(Ref<UI::Composer> Composer)
    {
        Composer.SetNextWindowPos(Composer.GetViewportCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
        Composer.SetNextWindowSize(800.0f, 600.0f, ImGuiCond_FirstUseEver);
        Composer.SetNextWindowSizeConstraints(320.0f, 200.0f);

        if (Composer.Begin(GetTitle(), mVisible))
        {
            Composer.BeginChild("##mainbar", ImVec2(0.0f, 40.0f), ImGuiChildFlags_Borders);
            DrawToolbar(Composer);
            Composer.EndChild();

            Composer.BeginChild("##viewport", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
            DrawViewport(Composer);
            Composer.EndChild();
        }
        Composer.End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scene::DrawToolbar(Ref<UI::Composer> Composer)
    {
        // Draw mode switch button to toggle between tile editing and entity editing modes.
        switch (mWorkshop.GetMode())
        {
        case Workshop::Mode::Tile:
            if (Composer.Button(ICON_FA_MAP "##mode", 32.0f))
            {
                mWorkshop.SetMode(Workshop::Mode::Entity);
            }
            break;
        case Workshop::Mode::Entity:
            if (Composer.Button(ICON_FA_CUBE "##mode", 32.0f))
            {
                mWorkshop.SetMode(Workshop::Mode::Tile);
            }
            break;
        }
        Composer.SameLine();


        // Draw common toolbar elements that are always visible.
        if (Composer.Button(ICON_FA_MAGNIFYING_GLASS_PLUS "##zoom_in", 32.0f))
        {
            const Real32 Current = GetContext().GetDirector().GetZoom();
            GetContext().GetDirector().SetZoom(Current * 1.1f);
        }
        Composer.SameLine();

        if (Composer.Button(ICON_FA_MAGNIFYING_GLASS_MINUS "##zoom_out", 32.0f))
        {
            const Real32 Current = GetContext().GetDirector().GetZoom();
            GetContext().GetDirector().SetZoom(Current * 0.9f);
        }
        Composer.SameLine();

        if (Composer.Button(ICON_FA_HOUSE "##reset_view", 32.0f))
        {
            GetContext().GetDirector().SetZoom(1.0f);
        }
        Composer.SameLine();

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

        // Draw projection mode and frame selector combos on the far right of the toolbar.
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

        Composer.SetCursorPosX(Composer.GetWindowWidth() - 200.0f);
        Composer.SetNextItemWidth(96.0f);

        const Director::Mode CurrentMode = GetContext().GetDirector().GetMode();

        if (Composer.BeginCombo("##mode", Enum::Name(CurrentMode)))
        {
            for (const Director::Mode Type : Enum::Values<Director::Mode>())
            {
                const Bool Selected = (CurrentMode == Type);

                if (Composer.Selectable(Enum::Name(Type), Selected))
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

        if (Composer.BeginCombo("##frame", Enum::Name(mFrame)))
        {
            for (const Renderer::Frame Type : Enum::Values<Renderer::Frame>())
            {
                const Bool Selected = (mFrame == Type);

                if (Composer.Selectable(Enum::Name(Type), Selected))
                {
                    mFrame = Type;
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

    void Scene::DrawTileToolbar(Ref<UI::Composer> Composer)
    {
        const auto DrawBrushButton = [&](Workshop::Brush Brush, ConstStr8 Icon)
        {
            const Bool Active = (mWorkshop.GetBrush() == Brush);

            if (Active)
            {
                Composer.PushStyleColor(ImGuiCol_Button,        Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
                Composer.PushStyleColor(ImGuiCol_ButtonHovered, Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
            }

            if (Composer.Button(Base::Format("{}##{}", Icon, Enum::Name(Brush)), 32.0f))
            {
                mWorkshop.SetBrush(Brush);
            }

            if (Active)
            {
                Composer.PopStyleColor(2);
            }
        };

        DrawBrushButton(Workshop::Brush::Select, ICON_FA_HAND);
        Composer.SameLine();

        DrawBrushButton(Workshop::Brush::Hand, ICON_FA_ARROW_POINTER);
        Composer.SameLine();

        DrawBrushButton(Workshop::Brush::Pencil, ICON_FA_BRUSH);
        Composer.SameLine();

        DrawBrushButton(Workshop::Brush::Bucket, ICON_FA_FILL_DRIP);
        Composer.SameLine();

        Composer.SeparatorEx(ImGuiSeparatorFlags_Vertical);
        Composer.SameLine();

        // Draw the layer selection buttons for switching between layers.
        const auto DrawLayerButton = [&](Workshop::Level Level, ConstStr8 Icon)
        {
            const Bool Active = (mWorkshop.GetLevel() == Level);

            if (Active)
            {
                Composer.PushStyleColor(ImGuiCol_Button,        Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
                Composer.PushStyleColor(ImGuiCol_ButtonHovered, Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
            }

            if (Composer.Button(Base::Format("{}##{}", Icon, Enum::Name(Level)), 32.0f))
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

    void Scene::DrawEntityToolbar(Ref<UI::Composer> Composer)
    {
        // TODO: Implement Entities.
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scene::DrawViewport(Ref<UI::Composer> Composer)
    {
        Ref<Renderer> Renderer = GetContext().GetRenderer();
        Ref<Director> Director = GetContext().GetDirector();

        // Draw the current frame rendered by the renderer to the interface.
        const Graphic::Object Texture = Renderer.GetFrame(mFrame);
        Composer.Image(Texture, Composer.GetContentRegionAvail());

        // Handle interactions with the viewport, such as hovering and clicking to manipulate the scene.
        if (ImGui::IsItemHovered())
        {
            const UInt32 AbsoluteX  = Composer.GetMousePos().x - Composer.GetItemRectMin().x;
            const Real32 NormalizeX = AbsoluteX / Composer.GetItemRectSize().x;
            const UInt32 AbsoluteY  = Composer.GetMousePos().y - Composer.GetItemRectMin().y;
            const Real32 NormalizeY = AbsoluteY / Composer.GetItemRectSize().y;

            // Handle mouse wheel input for zooming towards the cursor position.
            if (const Real32 Wheel = Composer.GetMouseWheel(); Wheel != 0.0f)
            {
                constexpr Real32 kFactor = 1.25f;

                const Real32  Zoom = Director.GetZoom() * (Wheel > 0.0f ? kFactor : (1.0f / kFactor));
                const Vector2 Pixel(
                    NormalizeX * Director.GetViewport().GetX() * Director.GetDensity(),
                    NormalizeY * Director.GetViewport().GetY() * Director.GetDensity());
                Director.Focus(Director.GetWorldCoordinates(Pixel), Zoom);
            }

            // Handle mouse dragging for panning the camera when the hand brush is selected.
            if (mWorkshop.GetBrush() == Workshop::Brush::Hand)
            {
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

                // Get the currently selected tile from the context.
                const UInt16 Selection = GetContext().GetInteger("Selection.Tile", 0);

                // Handle left-click for adding and right-click for removing tiles.
                if (IsRightButton || (IsLeftButton && Selection != 0))
                {
                    const Workshop::Command Command = IsLeftButton
                        ? Workshop::Command::Add
                        : Workshop::Command::Remove;
                    mWorkshop.Execute(Command, Director.GetWorldCoordinates(Vector2(AbsoluteX, AbsoluteY)), Selection);
                }
            }
        }
    }
}