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

namespace Tileon::Editor::Panel
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool CollectBounds(Scene::Entity Actor, Ref<IntRect> Result, Bool Started)
    {
        if (const ConstPtr<Tileon::Bound> Volume = Actor.TryGet<const Tileon::Bound>())
        {
            if (const IntRect Rect = Volume->GetRect(); !Rect.IsAlmostZero())
            {
                Result  = Started ? IntRect::Union(Result, Rect) : Rect;
                Started = true;
            }
        }

        Actor.Children([&](auto Child)
        {
            Started = CollectBounds(Scene::Entity(Child), Result, Started);
        });
        return Started;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Atelier::Atelier(Ref<Context> Context)
        : Activity      { Context, kTitle, true  },
          mWorkshop     { Context },
          mGizmo        { Context },
          mTarget       { Renderer::Target::Albedo },
          mMarquee      { false },
          mMarqueeMoved { false },
          mPaintTileX   { INT32_MIN },
          mPaintTileY   { INT32_MIN }
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
            Composer.Tooltip("Switch to entity mode");
            break;
        case Workshop::Mode::Entity:
            if (Composer.Button(ICON_FA_CUBE "##mode", 32.0f))
            {
                mWorkshop.SetBrush(Workshop::Brush::Pencil);
                mWorkshop.SetMode(Workshop::Mode::Tile);
            }
            Composer.Tooltip("Switch to tile mode");
            break;
        }
        Composer.SameLine();

        // Draw common toolbar elements that are always visible.
        if (Composer.Button(ICON_FA_MAGNIFYING_GLASS_PLUS "##zoom_in", 32.0f))
        {
            const Real32 Current = GetContext().GetDirector().GetZoom();
            GetContext().GetDirector().SetZoom(Current * 0.9f);
        }
        Composer.Tooltip("Zoom in");
        Composer.SameLine();

        if (Composer.Button(ICON_FA_MAGNIFYING_GLASS_MINUS "##zoom_out", 32.0f))
        {
            const Real32 Current = GetContext().GetDirector().GetZoom();
            GetContext().GetDirector().SetZoom(Current * 1.1f);
        }
        Composer.Tooltip("Zoom out");
        Composer.SameLine();

        if (Composer.Button(ICON_FA_HOUSE "##reset_view", 32.0f))
        {
            GetContext().GetDirector().SetZoom(1.0f);
        }
        Composer.Tooltip("Reset zoom");
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
            Composer.Tooltip("Camera X — press Enter to jump");
            Composer.SameLine();

            Composer.Label("Y");
            Composer.SameLine();
            Composer.SetNextItemWidth(72.0f);
            GoTo |= Composer.InputFloat("##camera_y", CameraY, 0.0f, 0.0f, "%.1f", ImGuiInputTextFlags_EnterReturnsTrue);
            Composer.Tooltip("Camera Y — press Enter to jump");
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
        DrawDebugButton(Composer, Renderer::Debug::Grid, ICON_FA_BORDER_ALL, "Toggle grid overlay");
        Composer.SameLine();

        DrawDebugButton(Composer, Renderer::Debug::Boundaries, ICON_FA_OBJECT_GROUP, "Toggle boundaries overlay");
        Composer.SameLine();

        // Draw projection mode and frame selector combos on the far right of the toolbar.
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

        Composer.SetCursorPosX(Composer.GetWindowWidth() - 200.0f);
        Composer.SetNextItemWidth(96.0f);

        const Director::Mode CurrentMode = GetContext().GetDirector().GetMode();

        if (Composer.BeginCombo("##projection", Enum::GetName(CurrentMode)))
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
        Composer.Tooltip("Projection mode");

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

                    GetContext().GetRenderer().SetOutput(Type);
                }

                if (Selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            Composer.EndCombo();
        }
        Composer.Tooltip("Displayed render target");

        ImGui::PopStyleVar();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawBrushButton(Ref<UI::Composer> Composer, Workshop::Brush Brush, Text Icon, Text Tooltip)
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

        Composer.Tooltip(Tooltip);

        if (Active)
        {
            Composer.PopStyleColor(2);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawDebugButton(Ref<UI::Composer> Composer, Renderer::Debug Overlay, Text Icon, Text Tooltip)
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

        Composer.Tooltip(Tooltip);

        if (Active)
        {
            Composer.PopStyleColor(2);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawTileToolbar(Ref<UI::Composer> Composer)
    {
        DrawBrushButton(Composer, Workshop::Brush::Hand,   ICON_FA_HAND,          "Pan the view");
        Composer.SameLine();

        DrawBrushButton(Composer, Workshop::Brush::Select, ICON_FA_ARROW_POINTER, "Select");
        Composer.SameLine();

        DrawBrushButton(Composer, Workshop::Brush::Pencil, ICON_FA_BRUSH,         "Paint tiles");
        Composer.SameLine();

        DrawBrushButton(Composer, Workshop::Brush::Bucket, ICON_FA_FILL_DRIP,     "Fill the region");
        Composer.SameLine();

        Composer.SeparatorEx(ImGuiSeparatorFlags_Vertical);
        Composer.SameLine();

        // Draw the layer selection buttons for switching between layers.
        const auto DrawLayerButton = [&](Workshop::Level Level, Text Icon, Text Hint)
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

            Composer.Tooltip(Hint);

            if (Active)
            {
                Composer.PopStyleColor(2);
            }
        };

        ImGui::AlignTextToFramePadding();
        ImGui::Text(ICON_FA_LAYER_GROUP);
        Composer.SameLine();

        DrawLayerButton(Workshop::Level::Base,   ICON_FA_1, "Base layer");
        Composer.SameLine();

        DrawLayerButton(Workshop::Level::Detail, ICON_FA_2, "Detail layer");
        Composer.SameLine();

        Composer.SeparatorEx(ImGuiSeparatorFlags_Vertical);
        Composer.SameLine();

        const Bool Seamless = mWorkshop.IsSeamless();

        if (Seamless)
        {
            Composer.PushStyleColor(ImGuiCol_Button,        Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
            Composer.PushStyleColor(ImGuiCol_ButtonHovered, Composer.GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        if (Composer.Button(ICON_FA_MAGNET "##align", 32.0f))
        {
            mWorkshop.SetSeamless(!Seamless);
        }

        Composer.Tooltip(Seamless ? "Grid-aligned (seamless terrain)"_Text : "Free stamp (precise placement)"_Text);

        if (Seamless)
        {
            Composer.PopStyleColor(2);
        }
        Composer.SameLine();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawEntityToolbar(Ref<UI::Composer> Composer)
    {
        // Entities are placed one at a time, so the area-filling bucket has no meaning here.
        DrawBrushButton(Composer, Workshop::Brush::Hand,   ICON_FA_HAND,          "Pan the view");
        Composer.SameLine();

        DrawBrushButton(Composer, Workshop::Brush::Select, ICON_FA_ARROW_POINTER, "Select");
        Composer.SameLine();

        DrawBrushButton(Composer, Workshop::Brush::Pencil, ICON_FA_BRUSH,         "Place entity");
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

        // Keep the multi-selection set consistent with single-selection changes made in Hierarchy or Inspector.
        mWorkshop.ReconcileSelection();

        const Lens Lens(Director, ViewportOrigin, ViewportSize);

        // Mark every selected entity with corner brackets, drawn as an overlay so they show under any brush.
        {
            const auto DrawBrackets = [&](IntRect AABB)
            {
                const ImVec2 Corner[4] =
                {
                    Lens.Project(Placement(0, 0, AABB.GetMinimumX(), AABB.GetMinimumY())),
                    Lens.Project(Placement(0, 0, AABB.GetMaximumX(), AABB.GetMinimumY())),
                    Lens.Project(Placement(0, 0, AABB.GetMaximumX(), AABB.GetMaximumY())),
                    Lens.Project(Placement(0, 0, AABB.GetMinimumX(), AABB.GetMaximumY())),
                };

                constexpr UInt32 Color = IM_COL32(255, 170, 40, 235);
                constexpr Real32 Thick = 2.0f;

                const Ptr<ImDrawList> List = ImGui::GetWindowDrawList();

                // At each corner, draw a short arm toward each of its two neighbours along the quad edges.
                const auto DrawArm = [&](ImVec2 From, ImVec2 To)
                {
                    const Real32 Length = Sqrt((To.x - From.x) * (To.x - From.x) + (To.y - From.y) * (To.y - From.y));
                    const Real32 Reach  = Length > 0.0001f ? Min(24.0f, Length * 0.35f) / Length : 0.0f;

                    List->AddLine(From, ImVec2(From.x + (To.x - From.x) * Reach, From.y + (To.y - From.y) * Reach), Color, Thick);
                };

                for (UInt32 Index = 0; Index < 4; ++Index)
                {
                    DrawArm(Corner[Index], Corner[(Index + 1) % 4]);
                    DrawArm(Corner[Index], Corner[(Index + 3) % 4]);
                }
            };

            const auto DrawSelected = [&](Scene::Entity Actor)
            {
                IntRect Bounds = IntRect::Zero();

                if (Actor.IsValid() && CollectBounds(Actor, Bounds, false))
                {
                    DrawBrackets(Bounds);
                }
            };

            if (ConstRef<Bag<UInt64>> Selection = mWorkshop.GetSelection(); !Selection.IsEmpty())
            {
                for (const UInt64 ID : Selection)
                {
                    DrawSelected(GetContext().GetScene().GetEntity(ID));
                }
            }
            else if (const UInt64 Selected = GetContext().GetInteger("Selection.Entity", 0))
            {
                DrawSelected(GetContext().GetScene().GetEntity(Selected));
            }
        }

        // Clipboard and delete shortcuts act on the whole selection whenever the viewport holds focus.
        if (ImGui::IsWindowFocused())
        {
            const Bool Control = ImGui::GetIO().KeyCtrl;

            if (Control && Composer.IsKeyPressed(ImGuiKey_C))
            {
                mWorkshop.CopySelection();
            }
            else if (Control && Composer.IsKeyPressed(ImGuiKey_X))
            {
                mWorkshop.CutSelection();
            }

            if (Composer.IsKeyPressed(ImGuiKey_Delete))
            {
                mWorkshop.DeleteSelection();
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

            // A marquee drag must own the cursor outright, so the handles stand down while one is in progress.
            if (!mMarquee)
            {
                Manipulating = mGizmo.Draw(Composer, mWorkshop.GetSelection(), Actor, ViewportOrigin, ViewportSize);
            }
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
            else if (mWorkshop.GetBrush() == Workshop::Brush::Select)
            {
                mWorkshop.ClearPreview();

                const Placement Cursor = Director.GetWorldCoordinates(Vector2(AbsoluteX, AbsoluteY));
                const Bool      Shift  = ImGui::GetIO().KeyShift;

                // Paste the clipboard's group so its anchor lands on the cursor.
                if (ImGui::GetIO().KeyCtrl && Composer.IsKeyPressed(ImGuiKey_V))
                {
                    mWorkshop.Paste(Cursor);
                }

                // A press starts either a marquee or a plain click; the drag distance decides which on release.
                if (Composer.IsMouseClicked(ImGuiMouseButton_Left))
                {
                    mMarquee       = true;
                    mMarqueeMoved  = false;
                    mMarqueeScreen = Composer.GetMousePos();
                }

                if (mMarquee && ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    const ImVec2 Now = Composer.GetMousePos();

                    if (Abs(Now.x - mMarqueeScreen.x) + Abs(Now.y - mMarqueeScreen.y) > 4.0f)
                    {
                        mMarqueeMoved = true;
                    }

                    if (mMarqueeMoved)
                    {
                        const ImVec2 Lower(Min(mMarqueeScreen.x, Now.x), Min(mMarqueeScreen.y, Now.y));
                        const ImVec2 Upper(Max(mMarqueeScreen.x, Now.x), Max(mMarqueeScreen.y, Now.y));

                        const Ptr<ImDrawList> List = ImGui::GetWindowDrawList();
                        List->AddRectFilled(Lower, Upper, IM_COL32(255, 170, 40, 40));
                        List->AddRect(Lower, Upper, IM_COL32(255, 170, 40, 200));
                    }
                }

                if (mMarquee && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    mMarquee = false;

                    if (mMarqueeMoved)
                    {
                        const ImVec2 Release = Composer.GetMousePos();
                        const ImVec2 Lower(Min(mMarqueeScreen.x, Release.x), Min(mMarqueeScreen.y, Release.y));
                        const ImVec2 Upper(Max(mMarqueeScreen.x, Release.x), Max(mMarqueeScreen.y, Release.y));

                        mWorkshop.SelectWithin(Lens, Lower, Upper, Shift);
                    }
                    else if (Shift)
                    {
                        mWorkshop.SelectToggle(Cursor);
                    }
                    else
                    {
                        mWorkshop.SelectSingle(Cursor);
                    }
                }
            }
            else if (mWorkshop.GetMode() == Workshop::Mode::Tile)
            {
                mWorkshop.ClearPreview();

                const UInt32    Selection = GetContext().GetInteger("Selection.Tile", 0);
                const Placement Cursor    = Director.GetWorldCoordinates(Vector2(AbsoluteX, AbsoluteY));

                // Footprint preview: show exactly which cells the stamp covers before it is committed.
                {
                    IntRect Footprint = IntRect::Zero();

                    if (mWorkshop.GetBrush() == Workshop::Brush::Bucket)
                    {
                        Footprint = IntRect(
                            Cursor.GetBaseX(), Cursor.GetBaseY(),
                            Cursor.GetBaseX() + Tileon::Region::kTilesPerX,
                            Cursor.GetBaseY() + Tileon::Region::kTilesPerY);
                    }
                    else
                    {
                        const IntVector2 Span  = Selection ? mContext.GetTileset().GetMotif(Selection).GetSpan() : IntVector2::One();
                        const SInt32     TileX = static_cast<SInt32>(Floor(Cursor.GetAbsoluteX()));
                        const SInt32     TileY = static_cast<SInt32>(Floor(Cursor.GetAbsoluteY()));

                        Footprint = IntRect(TileX, TileY, TileX + Span.GetX(), TileY + Span.GetY());
                    }

                    const ImVec2 Corner0 = Lens.Project(Placement(0, 0, Footprint.GetMinimumX(), Footprint.GetMinimumY()));
                    const ImVec2 Corner1 = Lens.Project(Placement(0, 0, Footprint.GetMaximumX(), Footprint.GetMinimumY()));
                    const ImVec2 Corner2 = Lens.Project(Placement(0, 0, Footprint.GetMaximumX(), Footprint.GetMaximumY()));
                    const ImVec2 Corner3 = Lens.Project(Placement(0, 0, Footprint.GetMinimumX(), Footprint.GetMaximumY()));

                    const Ptr<ImDrawList> List = ImGui::GetWindowDrawList();
                    List->AddQuadFilled(Corner0, Corner1, Corner2, Corner3, IM_COL32(120, 200, 255, 45));
                    List->AddQuad(Corner0, Corner1, Corner2, Corner3, IM_COL32(120, 200, 255, 220), 1.5f);
                }

                // Pencil paints a continuous stroke while held; Bucket fills once per click.
                const Bool   Continuous = (mWorkshop.GetBrush() == Workshop::Brush::Pencil);
                const SInt32 TileX      = static_cast<SInt32>(Floor(Cursor.GetAbsoluteX()));
                const SInt32 TileY      = static_cast<SInt32>(Floor(Cursor.GetAbsoluteY()));
                const Bool   NewTile    = (TileX != mPaintTileX) || (TileY != mPaintTileY);

                const Bool LeftClick  = Composer.IsMouseClicked(ImGuiMouseButton_Left);
                const Bool RightClick = Composer.IsMouseClicked(ImGuiMouseButton_Right);
                const Bool LeftHeld   = ImGui::IsMouseDown(ImGuiMouseButton_Left);
                const Bool RightHeld  = ImGui::IsMouseDown(ImGuiMouseButton_Right);

                const Bool Erase = RightClick || (Continuous && RightHeld && NewTile);
                const Bool Paint = (LeftClick || (Continuous && LeftHeld && NewTile)) && Selection != 0;

                if (Erase)
                {
                    mWorkshop.Execute(Workshop::Command::Remove, Cursor, Selection);
                }
                else if (Paint)
                {
                    mWorkshop.Execute(Workshop::Command::Add, Cursor, Selection);
                }

                if (LeftHeld || RightHeld)
                {
                    mPaintTileX = TileX;
                    mPaintTileY = TileY;
                }
            }
            else
            {
                const Bool IsLeftButton  = Composer.IsMouseClicked(ImGuiMouseButton_Left);
                const Bool IsRightButton = Composer.IsMouseClicked(ImGuiMouseButton_Right);

                const UInt32    Selection = GetContext().GetInteger("Selection.Archetype", 0);
                const Placement Cursor    = Director.GetWorldCoordinates(Vector2(AbsoluteX, AbsoluteY));

                // Show the pending entity under the cursor before it is committed by a click.
                mWorkshop.UpdatePreview(Cursor, Selection);

                // Handle left-click for adding and right-click for removing entities.
                if (IsRightButton || (IsLeftButton && Selection != 0))
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