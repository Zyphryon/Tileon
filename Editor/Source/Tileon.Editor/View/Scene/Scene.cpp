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
          mFrame    { Renderer::Frame::Final }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Scene::OnDraw(Ref<UI::Composer> Composer)
    {
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
        Composer.PushStyleColor(ImGuiCol_Button,        Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
        Composer.PushStyleColor(ImGuiCol_ButtonHovered, Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
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
        Composer.PopStyleColor(2);
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

        // Draw frame selector combo on the far right of the toolbar.
        Composer.SetCursorPosX(Composer.GetWindowWidth() - 100.0f);
        Composer.SetNextItemWidth(96.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

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
        if (Composer.Button(ICON_FA_HAND "##select", 32.0f))
        {
            mWorkshop.SetBrush(Workshop::Brush::Select);
        }
        Composer.SameLine();

        if (Composer.Button(ICON_FA_ARROW_POINTER "##pan", 32.0f))
        {
            mWorkshop.SetBrush(Workshop::Brush::Hand);
        }
        Composer.SameLine();

        if (Composer.Button(ICON_FA_BRUSH "##paint", 32.0f))
        {
            mWorkshop.SetBrush(Workshop::Brush::Pencil);
        }
        Composer.SameLine();

        if (Composer.Button(ICON_FA_ERASER "##erase", 32.0f))
        {
            mWorkshop.SetBrush(Workshop::Brush::Pencil);
        }
        Composer.SameLine();

        if (Composer.Button(ICON_FA_FILL_DRIP "##fill", 32.0f))
        {
            mWorkshop.SetBrush(Workshop::Brush::Bucket);
        }
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
                    const ImVec2    Delta   = Composer.GetMouseDelta();
                    const ImVec2    Size    = Composer.GetItemRectSize();
                    const Placement Current = Director.GetPosition();

                    const Real32 ScaleX = (Director.GetZoom() * Director.GetViewport().GetX() / Size.x);
                    const Real32 ScaleY = (Director.GetZoom() * Director.GetViewport().GetY() / Size.y);

                    const Placement Target = Placement::FromAbsolute(
                        Current.GetAbsoluteX() - Delta.x * ScaleX,
                        Current.GetAbsoluteY() + Delta.y * ScaleY);
                    Director.SetPosition(Target);
                }
            }
            else
            {
                const Bool IsLeftButton  = Composer.IsMouseClicked(ImGuiMouseButton_Left);
                const Bool IsRightButton = Composer.IsMouseClicked(ImGuiMouseButton_Right);

                // Handle mouse clicks for painting or erasing tiles when the pencil or bucket brush is selected.
                if (IsLeftButton != IsRightButton)
                {
                    const Workshop::Command Command = IsLeftButton
                        ? Workshop::Command::Add
                        : Workshop::Command::Remove;
                    mWorkshop.Execute(Command, Director.GetWorldCoordinates(Vector2(AbsoluteX, AbsoluteY)));
                }
            }
        }
    }
}