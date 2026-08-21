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

#include "Viewport.hpp"
#include "Tileon.Editor/Toolkit/Theme.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool CollectBounds(ConstRef<Tileon::Projection> Projection, Scene::Entity Actor, Ref<IntRect> Result, Bool Started)
    {
        if (const ConstPtr<Tileon::Enclosure> Volume = Actor.TryGet<const Tileon::Enclosure>())
        {
            // The brackets wrap what the entity covers on screen, which is the projection's business now.
            const IntRect Rect = Rect::Enclose<SInt32>(Projection.Project(Volume->GetVolume()));

            if (!Rect.IsAlmostZero())
            {
                Result  = Started ? IntRect::Union(Result, Rect) : Rect;
                Started = true;
            }
        }

        Actor.Children([&](auto Child)
        {
            Started = CollectBounds(Projection, Scene::Entity(Child), Result, Started);
        });
        return Started;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Viewport::Viewport(Ref<Context> Context)
        : Panel         { Context, kTitle, true  },
          mTools        { Context },
          mGizmo        { Context },
          mTarget       { Renderer::Target::Albedo },
          mTimescale    { 1.0f },
          mYaw          { 0.0f },
          mTilt         { kMaxTilt },
          mMarquee      { false },
          mMarqueeMoved { false },
          mStroke       { false },
          mPaintUnitX   { 0 },
          mPaintUnitY   { 0 },
          mPaintTime    { 0.0 }
    {
        // The preview phase resolves one target, so it starts on the one the viewport opens with.
        Context.GetRenderer().SetOutput(mTarget);

        // A light draws nothing of its own, so the viewport marks where each one sits.
        Ref<Scene::Service> Scene = Context.GetScene();

        mQrGlowlights = Scene.CreateQuery<
            Scene::DSL::With<Tileon::Glowlight>, Scene::DSL::In<const Tileon::Transform, ConstPtr<IntColor8>>
        >("Editor::Viewport::Glowlights", Scene::Cache::Auto);

        mQrSpotlights = Scene.CreateQuery<
            Scene::DSL::With<Tileon::Spotlight>, Scene::DSL::In<const Tileon::Transform, ConstPtr<IntColor8>>
        >("Editor::Viewport::Spotlights", Scene::Cache::Auto);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Viewport::OnDraw()
    {
        // A stroke that landed on a region the world was still bringing in is laid down once it arrives.
        mTools.GetGround().Flush();

        // A stroke ends wherever the button is let go, which may well be outside the viewport it started in.
        if (mStroke
            && !Toolkit::Composer::IsMouseDown(ImGuiMouseButton_Left)
            && !Toolkit::Composer::IsMouseDown(ImGuiMouseButton_Right))
        {
            mStroke = false;

            GetContext().GetHistory().Close();
        }

        Toolkit::Composer::SetNextWindowPos(Toolkit::Composer::GetViewportCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
        Toolkit::Composer::SetNextWindowSize(800.0f, 600.0f, ImGuiCond_FirstUseEver);
        Toolkit::Composer::SetNextWindowSizeConstraints(320.0f, 200.0f);

        if (Toolkit::Composer::Begin(GetTitle(), mVisible))
        {
            Toolkit::Composer::BeginChild("##mainbar", ImVec2(0.0f, 40.0f), ImGuiChildFlags_Borders);
            DrawToolbar();
            Toolkit::Composer::EndChild();

            Toolkit::Composer::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            Toolkit::Composer::BeginChild("##viewport", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
            Toolkit::Composer::PopStyleColor();

            DrawViewport();
            Toolkit::Composer::EndChild();
        }
        else
        {
            mTools.GetEntities().ClearPreview();
        }
        Toolkit::Composer::End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Viewport::DrawToolbar()
    {

        // Draw common toolbar elements that are always visible.
        if (Toolkit::Composer::Button(ICON_FA_MAGNIFYING_GLASS_PLUS "##zoom_in", 32.0f))
        {
            const Real32 Current = GetContext().GetDirector().GetZoom();
            GetContext().GetDirector().SetZoom(Current * 0.9f);
        }
        Toolkit::Composer::Tooltip("Zoom in");
        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::Button(ICON_FA_MAGNIFYING_GLASS_MINUS "##zoom_out", 32.0f))
        {
            const Real32 Current = GetContext().GetDirector().GetZoom();
            GetContext().GetDirector().SetZoom(Current * 1.1f);
        }
        Toolkit::Composer::Tooltip("Zoom out");
        Toolkit::Composer::SameLine();

        if (Toolkit::Composer::Button(ICON_FA_HOUSE "##reset_view", 32.0f))
        {
            GetContext().GetDirector().SetZoom(1.0f);
        }
        Toolkit::Composer::Tooltip("Reset zoom");
        Toolkit::Composer::SameLine();

        // Camera position read-out that doubles as a "go to": shows the view center in absolute world tiles and
        // re centers the camera when a new coordinate is committed (Enter).
        Ref<Director>   Director = GetContext().GetDirector();
        const Placement Center   = Director.GetPosition();

        Real32 CameraX = static_cast<Real32>(Center.GetAbsoluteX());
        Real32 CameraY = static_cast<Real32>(Center.GetAbsoluteY());

        Bool GoTo = false;

        Toolkit::Composer::Label("X");
        Toolkit::Composer::SameLine();
        Toolkit::Composer::SetNextItemWidth(72.0f);
        Toolkit::Composer::InputFloat("##camera_x", CameraX, 0.0f, 0.0f, "%.1f", ImGuiInputTextFlags_EnterReturnsTrue);
        GoTo |= Toolkit::Composer::IsItemDeactivatedAfterEdit();
        Toolkit::Composer::Tooltip("Camera X — press Enter or leave the field to jump");
        Toolkit::Composer::SameLine();

        Toolkit::Composer::Label("Y");
        Toolkit::Composer::SameLine();
        Toolkit::Composer::SetNextItemWidth(72.0f);
        Toolkit::Composer::InputFloat("##camera_y", CameraY, 0.0f, 0.0f, "%.1f", ImGuiInputTextFlags_EnterReturnsTrue);
        GoTo |= Toolkit::Composer::IsItemDeactivatedAfterEdit();
        Toolkit::Composer::Tooltip("Camera Y — press Enter or leave the field to jump");
        Toolkit::Composer::SameLine();

        if (GoTo)
        {
            Director.SetPosition(Placement::FromAbsolute(CameraX, CameraY));
        }

        Toolkit::Composer::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        Toolkit::Composer::SameLine();

        // The ground and the entities over it are painted with different tools, so the bar follows the mode.
        switch (mTools.GetMode())
        {
        case Tools::Mode::Ground:
            DrawGroundToolbar();
            break;
        case Tools::Mode::Entity:
            DrawEntityToolbar();
            break;
        }
        Toolkit::Composer::SameLine();

        Toolkit::Composer::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        Toolkit::Composer::SameLine();

        // Draw the diagnostic overlay toggles.
        DrawDebugButton(Renderer::Debug::Grid, ICON_FA_BORDER_ALL, "Toggle grid overlay");
        Toolkit::Composer::SameLine();

        DrawDebugButton(Renderer::Debug::Boundaries, ICON_FA_OBJECT_GROUP, "Toggle boundaries overlay");
        Toolkit::Composer::SameLine();

        Toolkit::Composer::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        Toolkit::Composer::SameLine();

        DrawTimescaleToolbar();
        Toolkit::Composer::SameLine();

        // Draw projection mode and frame selector combos on the far right of the toolbar.
        Toolkit::Composer::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

        Toolkit::Composer::SetCursorPosX(Toolkit::Composer::GetWindowWidth() - 200.0f);
        Toolkit::Composer::SetNextItemWidth(96.0f);

        Perspective CurrentMode = GetContext().GetEnum("Viewport.Perspective", Perspective::Ortho);

        if (Toolkit::Composer::Combo("##projection", CurrentMode))
        {
            GetContext().SetEnum("Viewport.Perspective", CurrentMode);

            switch (CurrentMode)
            {
            case Perspective::Isometric:
                GetContext().GetDirector().SetProjection(Tileon::Projection::Isometric());
                break;
            case Perspective::Axonometric:
                GetContext().GetDirector().SetProjection(
                    Tileon::Projection::Axonometric(Angle::FromDegrees(mYaw), mTilt));
                break;
            default:
                GetContext().GetDirector().SetProjection(Tileon::Projection::Ortho());
                break;
            }
        }
        Toolkit::Composer::Tooltip("Projection mode");

        Toolkit::Composer::SameLine();
        Toolkit::Composer::SetNextItemWidth(96.0f);

        if (Toolkit::Composer::Combo("##target", mTarget))
        {
            GetContext().GetRenderer().SetOutput(mTarget);
        }
        Toolkit::Composer::Tooltip("Displayed render target");

        Toolkit::Composer::PopStyleVar();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Viewport::DrawBrushButton(Tools::Brush Brush, Text Icon, Text Tooltip)
    {
        const Bool Active = (mTools.GetBrush() == Brush);

        if (Active)
        {
            Toolkit::Composer::PushStyleColor(ImGuiCol_Button,        Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
            Toolkit::Composer::PushStyleColor(ImGuiCol_ButtonHovered, Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        if (Toolkit::Composer::Button(String<64>::Print<"{0}##{1}">(Icon, Enum::GetName(Brush)), 32.0f))
        {
            mTools.SetBrush(Brush);
        }

        Toolkit::Composer::Tooltip(Tooltip);

        if (Active)
        {
            Toolkit::Composer::PopStyleColor(2);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Viewport::DrawShapeButton(Tools::Shape Shape, Text Icon, Text Tooltip)
    {
        const Bool Active = (mTools.GetGround().GetShape() == Shape);

        if (Active)
        {
            Toolkit::Composer::PushStyleColor(ImGuiCol_Button,        Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
            Toolkit::Composer::PushStyleColor(ImGuiCol_ButtonHovered, Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        if (Toolkit::Composer::Button(String<64>::Print<"{0}##{1}">(Icon, Enum::GetName(Shape)), 32.0f))
        {
            mTools.GetGround().SetShape(Shape);
        }

        Toolkit::Composer::Tooltip(Tooltip);

        if (Active)
        {
            Toolkit::Composer::PopStyleColor(2);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Viewport::DrawSelectionHint(ConstRef<Camera> Camera, Scene::Entity Actor)
    {
        if (!Actor.IsValid())
        {
            return;
        }

        const ConstPtr<Enclosure> Bounds = Actor.TryGet<const Enclosure>();

        if (!Bounds)
        {
            return;
        }

        // The box is already in absolute tiles, so its corners project straight onto the ground and the
        // height above it. An entity the spatial pass has not measured yet has nothing worth outlining.
        const IntBox Volume = Bounds->GetVolume();

        if (Volume.IsAlmostZero())
        {
            return;
        }

        const IntVector3 Lower = Volume.GetMinimum();
        const IntVector3 Upper = Volume.GetMaximum();

        const auto Corner = [&](SInt32 X, SInt32 Z, SInt32 Y)
        {
            return Camera.Project(Placement(0, 0, X, Z), static_cast<Real32>(Y));
        };

        const ImVec2 Floor[4] =
        {
            Corner(Lower.GetX(), Lower.GetZ(), Lower.GetY()), Corner(Upper.GetX(), Lower.GetZ(), Lower.GetY()),
            Corner(Upper.GetX(), Upper.GetZ(), Lower.GetY()), Corner(Lower.GetX(), Upper.GetZ(), Lower.GetY()),
        };
        const ImVec2 Roof[4] =
        {
            Corner(Lower.GetX(), Lower.GetZ(), Upper.GetY()), Corner(Upper.GetX(), Lower.GetZ(), Upper.GetY()),
            Corner(Upper.GetX(), Upper.GetZ(), Upper.GetY()), Corner(Lower.GetX(), Upper.GetZ(), Upper.GetY()),
        };

        constexpr UInt32 kTint = Toolkit::Theme::kHint;

        const Ptr<ImDrawList> List = Toolkit::Composer::GetWindowDrawList();

        List->AddQuad(Floor[0], Floor[1], Floor[2], Floor[3], kTint, 1.5f);
        List->AddQuad(Roof[0],  Roof[1],  Roof[2],  Roof[3],  kTint, 1.5f);

        for (UInt32 Edge = 0; Edge < 4; ++Edge)
        {
            List->AddLine(Floor[Edge], Roof[Edge], kTint, 1.5f);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Viewport::DrawTimescaleToolbar()
    {
        Ref<Scene::Service> Scene = GetContext().GetScene();

        // A paused scene keeps its speed in mTimescale, so resuming returns to whatever was selected.
        const Bool Paused = (Scene.GetTimescale() <= 0.0f);

        if (Toolkit::Composer::Button(Paused ? ICON_FA_PLAY "##timescale_pause" : ICON_FA_PAUSE "##timescale_pause", 32.0f))
        {
            Scene.SetTimescale(Paused ? mTimescale : 0.0f);
        }

        Toolkit::Composer::Tooltip(Paused ? "Resume the scene"_Text : "Pause the scene"_Text);
        Toolkit::Composer::SameLine();

        Toolkit::Composer::SetNextItemWidth(72.0f);

        if (Toolkit::Composer::BeginCombo("##timescale", String<16>::Print<"{0:.2f}x">(mTimescale)))
        {
            static constexpr Real32 kTimescales[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f };

            for (const Real32 Speed : kTimescales)
            {
                if (Toolkit::Composer::Selectable(String<16>::Print<"{0:.2f}x">(Speed), mTimescale == Speed))
                {
                    mTimescale = Speed;

                    // A speed picked while paused is remembered but not applied, so the scene stays still.
                    if (!Paused)
                    {
                        Scene.SetTimescale(Speed);
                    }
                }
            }
            Toolkit::Composer::EndCombo();
        }

        Toolkit::Composer::Tooltip("Speed the scene simulation runs at");
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Viewport::DrawDebugButton(Renderer::Debug Overlay, Text Icon, Text Tooltip)
    {
        const Bool Active = GetContext().GetRenderer().HasOverlay(Overlay);

        if (Active)
        {
            Toolkit::Composer::PushStyleColor(ImGuiCol_Button,        Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
            Toolkit::Composer::PushStyleColor(ImGuiCol_ButtonHovered, Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        if (Toolkit::Composer::Button(String<64>::Print<"{0}##{1}">(Icon, Enum::GetName(Overlay)), 32.0f))
        {
            GetContext().GetRenderer().SetOverlay(Overlay, !Active);
        }

        Toolkit::Composer::Tooltip(Tooltip);

        if (Active)
        {
            Toolkit::Composer::PopStyleColor(2);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Viewport::DrawGroundToolbar()
    {
        DrawBrushButton(Tools::Brush::Hand,   ICON_FA_HAND,          "Pan the view");
        Toolkit::Composer::SameLine();

        DrawBrushButton(Tools::Brush::Select, ICON_FA_ARROW_POINTER, "Select");
        Toolkit::Composer::SameLine();

        DrawBrushButton(Tools::Brush::Pencil, ICON_FA_BRUSH,         "Paint the ground");
        Toolkit::Composer::SameLine();

        DrawBrushButton(Tools::Brush::Bucket, ICON_FA_FILL_DRIP,     "Fill the region");
        Toolkit::Composer::SameLine();

        Toolkit::Composer::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        Toolkit::Composer::SameLine();

        DrawShapeButton(Tools::Shape::Square,  ICON_FA_SQUARE,  "Square brush");
        Toolkit::Composer::SameLine();

        DrawShapeButton(Tools::Shape::Circle,  ICON_FA_CIRCLE,  "Round brush");
        Toolkit::Composer::SameLine();

        DrawShapeButton(Tools::Shape::Diamond, ICON_FA_DIAMOND, "Diamond brush");
        Toolkit::Composer::SameLine();

        Toolkit::Composer::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        Toolkit::Composer::SameLine();

        SInt32 Size = mTools.GetGround().GetSize();

        Toolkit::Composer::SetNextItemWidth(96.0f);

        if (Toolkit::Composer::DragInt("##brush_size", Size, 0.2f, 1, Ground::kMaxSize))
        {
            mTools.GetGround().SetSize(static_cast<UInt8>(Size));
        }

        Toolkit::Composer::Tooltip("How far the brush reaches, in world units");
        Toolkit::Composer::SameLine();

        SInt32 Flow = (mTools.GetGround().GetFlow() * 100 + 127) / 255;

        Toolkit::Composer::SetNextItemWidth(96.0f);

        if (Toolkit::Composer::DragInt("##brush_flow", Flow, 1.0f, 1, 100, "%d%%"))
        {
            mTools.GetGround().SetFlow(static_cast<UInt8>(Flow * 255 / 100));
        }

        Toolkit::Composer::Tooltip("How much the brush lays down on each pass — work it in by going over it");
        Toolkit::Composer::SameLine();

        const Bool Soft = mTools.GetGround().IsSoft();

        if (Soft)
        {
            Toolkit::Composer::PushStyleColor(ImGuiCol_Button,        Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
            Toolkit::Composer::PushStyleColor(ImGuiCol_ButtonHovered, Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        if (Toolkit::Composer::Button(ICON_FA_FEATHER "##softness", 32.0f))
        {
            mTools.GetGround().SetSoft(!Soft);
        }

        Toolkit::Composer::Tooltip(Soft
            ? "Fade the terrain out towards the rim"_Text
            : "Lay the terrain down evenly, edge and all"_Text);

        if (Soft)
        {
            Toolkit::Composer::PopStyleColor(2);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Viewport::DrawEntityToolbar()
    {
        if (mTools.GetBrush() == Tools::Brush::Bucket)
        {
            mTools.SetBrush(Tools::Brush::Pencil);
        }

        // Entities are placed one at a time, so the area-filling bucket has no meaning here.
        DrawBrushButton(Tools::Brush::Hand,   ICON_FA_HAND,          "Pan the view");
        Toolkit::Composer::SameLine();

        DrawBrushButton(Tools::Brush::Select, ICON_FA_ARROW_POINTER, "Select");
        Toolkit::Composer::SameLine();

        DrawBrushButton(Tools::Brush::Pencil, ICON_FA_BRUSH,         "Place entity");
        Toolkit::Composer::SameLine();

        Toolkit::Composer::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        Toolkit::Composer::SameLine();

        // Snap placement to tile centers, so entities line up on the grid instead of landing at the exact cursor.
        const Bool Aligned = mTools.IsAligned();

        if (Aligned)
        {
            Toolkit::Composer::PushStyleColor(ImGuiCol_Button,        Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
            Toolkit::Composer::PushStyleColor(ImGuiCol_ButtonHovered, Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        if (Toolkit::Composer::Button(ICON_FA_CROSSHAIRS "##center", 32.0f))
        {
            mTools.SetAligned(!Aligned);
        }

        Toolkit::Composer::Tooltip(Aligned
            ? "Snap to tile center"_Text
            : "Free placement (hold Shift to center)"_Text);

        if (Aligned)
        {
            Toolkit::Composer::PopStyleColor(2);
        }
        Toolkit::Composer::SameLine();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Viewport::DrawLightMarkers(ConstRef<Camera> Camera)
    {
        const Ptr<ImDrawList> List = Toolkit::Composer::GetWindowDrawList();

        // A light's transform is region-local, so its origin is folded back in to place it in absolute tiles.
        const auto Locate = [&](ConstRef<Tileon::Transform> Transform)
        {
            return Transform.GetWorldspace().GetTranslation() + Vector3(Transform.GetOrigin());
        };

        const auto DrawIcon = [&](ImVec2 At, Text Icon, UInt32 Color)
        {
            const ImVec2 Size = Toolkit::Composer::CalcTextSize(Icon);
            List->AddText(ImVec2(At.x - Size.x * 0.5f, At.y - Size.y * 0.5f), Color, Icon.GetData());
        };

        const auto Shade = [](ConstPtr<IntColor8> Tint)
        {
            return Tint ? IM_COL32(Tint->GetRed(), Tint->GetGreen(), Tint->GetBlue(), 235) : Toolkit::Theme::kMarker;
        };

        mQrGlowlights.Run([&](ConstRef<Tileon::Transform> Transform, ConstPtr<IntColor8> Tint)
        {
            const Vector3 World = Locate(Transform);
            const ImVec2  At    = Camera.Project(Placement::FromAbsolute(World.GetX(), World.GetZ()), World.GetY());

            DrawStem(Camera, World, Toolkit::Theme::kMarkerFaint);
            DrawIcon(At, ICON_FA_LIGHTBULB, Shade(Tint));
        });

        mQrSpotlights.Run([&](ConstRef<Tileon::Transform> Transform, ConstPtr<IntColor8> Tint)
        {
            const Vector3 World = Locate(Transform);
            const ImVec2  At    = Camera.Project(Placement::FromAbsolute(World.GetX(), World.GetZ()), World.GetY());

            DrawStem(Camera, World, Toolkit::Theme::kMarkerFaint);
            DrawIcon(At, ICON_FA_FILTER, Shade(Tint));
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Viewport::DrawWorldRing(ConstRef<Camera> Camera, Vector3 Center, Vector3 AxisU, Vector3 AxisV, Real32 Radius, UInt32 Color)
    {
        constexpr UInt32 kSegments = 24;

        ImVec2 Ring[kSegments];

        for (UInt32 Step = 0; Step < kSegments; ++Step)
        {
            const Real32  Theta = (2.0f * kPI<Real32> * Step) / kSegments;
            const Vector3 Point = Center + (AxisU * Base::Cosine(Theta) + AxisV * Base::Sine(Theta)) * Radius;

            Ring[Step] = Camera.Project(Placement::FromAbsolute(Point.GetX(), Point.GetZ()), Point.GetY());
        }

        Toolkit::Composer::GetWindowDrawList()->AddPolyline(Ring, kSegments, Color, ImDrawFlags_Closed, 1.5f);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Viewport::DrawSpotlightCone(ConstRef<Camera> Camera, ConstRef<Tileon::Transform> Transform, ConstRef<Tileon::Spotlight> Light)
    {
        constexpr UInt32 kColor = Toolkit::Theme::kSelectSoft;

        const Vector3 Basis = Transform.GetWorldspace().GetBasisX();

        // A basis with no length has no direction to aim along, the same guard the light's own pass applies.
        if (Basis.GetLength() < 0.0001f)
        {
            return;
        }

        const Vector3 Aim   = Vector3::Normalize(Basis);
        const Vector3 World = Transform.GetWorldspace().GetTranslation() + Vector3(Transform.GetOrigin());

        // Any axis off the aim spans the end disc; the world's up serves unless the light points along it.
        const Vector3 Guide = Abs(Aim.GetY()) > 0.99f ? Vector3(1.0f, 0.0f, 0.0f) : Vector3(0.0f, 1.0f, 0.0f);
        const Vector3 Right = Vector3::Normalize(Vector3::Cross(Aim, Guide));
        const Vector3 Up    = Vector3::Cross(Right, Aim);

        const Vector3 Center = World + Aim * Light.GetRange();
        const Real32  Radius = Light.GetRange() * Angle::Tangent(Light.GetOuterAngle());

        DrawWorldRing(Camera, Center, Right, Up, Radius, kColor);

        // Four edges carry the spread without the mush eight would make at a shallow tilt.
        const Vector3 Rim[4] =
        {
            Center + Right * Radius, Center + Up * Radius, Center - Right * Radius, Center - Up * Radius,
        };

        const Ptr<ImDrawList> List = Toolkit::Composer::GetWindowDrawList();
        const ImVec2          Apex = Camera.Project(Placement::FromAbsolute(World.GetX(), World.GetZ()), World.GetY());

        for (UInt32 Edge = 0; Edge < 4; ++Edge)
        {
            const ImVec2 Tip = Camera.Project(Placement::FromAbsolute(Rim[Edge].GetX(), Rim[Edge].GetZ()), Rim[Edge].GetY());

            List->AddLine(Apex, Tip, kColor, 1.5f);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Viewport::DrawGlowlightArea(ConstRef<Camera> Camera, ConstRef<Tileon::Transform> Transform, ConstRef<Tileon::Glowlight> Light)
    {
        const Vector3 World = Transform.GetWorldspace().GetTranslation() + Vector3(Transform.GetOrigin());
        const Vector3 Scale = Transform.GetWorldspace().GetScale();

        const Real32 Radius = Light.GetRadius() * Max(Scale.GetX(), Max(Scale.GetY(), Scale.GetZ()));

        constexpr Vector3 AxisX = Vector3::UnitX();
        constexpr Vector3 AxisY = Vector3::UnitY();
        constexpr Vector3 AxisZ = Vector3::UnitZ();

        DrawWorldRing(Camera, World, AxisX, AxisZ, Radius, Toolkit::Theme::kSelectSoft);
        DrawWorldRing(Camera, World, AxisX, AxisY, Radius, Toolkit::Theme::kSelectSoft);
        DrawWorldRing(Camera, World, AxisY, AxisZ, Radius, Toolkit::Theme::kSelectSoft);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Viewport::DrawStem(ConstRef<Camera> Camera, Vector3 World, UInt32 Color)
    {
        constexpr Real32 kDash     = 2.0f;
        constexpr Real32 kStride   = 5.0f;
        constexpr Real32 kRadius   = 5.0f;
        constexpr UInt32 kSegments = 20;

        const Ptr<ImDrawList> List   = Toolkit::Composer::GetWindowDrawList();
        const Placement       Ground = Placement::FromAbsolute(World.GetX(), World.GetZ());
        const ImVec2          At     = Camera.Project(Ground, World.GetY());
        const ImVec2          Foot   = Camera.Project(Ground, 0.0f);

        const ImVec2 Span   = ImVec2(Foot.x - At.x, Foot.y - At.y);
        const Real32 Length = Sqrt(Span.x * Span.x + Span.y * Span.y);

        if (Length > kStride)
        {
            const ImVec2 Step(Span.x / Length, Span.y / Length);

            for (Real32 Walk = 0.0f; Walk < Length; Walk += kStride)
            {
                const Real32 Tail = Min(Walk + kDash, Length);

                List->AddLine(
                    ImVec2(At.x + Step.x * Walk, At.y + Step.y * Walk),
                    ImVec2(At.x + Step.x * Tail, At.y + Step.y * Tail), Color);
            }
        }

        const ImVec2 AxisX = Camera.Direction(Vector3(1.0f, 0.0f, 0.0f));
        const ImVec2 AxisZ = Camera.Direction(Vector3(0.0f, 0.0f, 1.0f));
        const Real32 Reach = Max(
            Sqrt(AxisX.x * AxisX.x + AxisX.y * AxisX.y),
            Sqrt(AxisZ.x * AxisZ.x + AxisZ.y * AxisZ.y));
        const Real32 Scale = kRadius / Max(Reach, 0.0001f);

        ImVec2 Ring[kSegments];

        for (UInt32 Step = 0; Step < kSegments; ++Step)
        {
            const Real32 Theta = (2.0f * kPI<Real32> * Step) / kSegments;
            const Real32 Cos   = Base::Cosine(Theta);
            const Real32 Sin   = Base::Sine(Theta);

            Ring[Step] = ImVec2(
                Foot.x + (AxisX.x * Cos + AxisZ.x * Sin) * Scale,
                Foot.y + (AxisX.y * Cos + AxisZ.y * Sin) * Scale);
        }

        List->AddPolyline(Ring, kSegments, Color, ImDrawFlags_Closed, 1.0f);
        List->AddCircleFilled(Foot, 1.5f, Color);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Viewport::DrawViewport()
    {
        Ref<Renderer> Renderer = GetContext().GetRenderer();
        Ref<Director> Director = GetContext().GetDirector();

        const Graphic::Object Texture = Renderer.GetTarget(Renderer::Target::Final);

        // OpenGL samples from the bottom-left, so its render target arrives flipped relative to Direct3D's.
        if (GetContext().GetGraphic().GetDescription().Language == Graphic::ShaderLanguage::GLSL)
        {
            Toolkit::Composer::Image(Texture, Toolkit::Composer::GetContentRegionAvail(), ImVec4(0, 1, 1, 0));
        }
        else
        {
            Toolkit::Composer::Image(Texture, Toolkit::Composer::GetContentRegionAvail(), ImVec4(0, 0, 1, 1));
        }

        const ImVec2 ViewportOrigin = Toolkit::Composer::GetItemRectMin();
        const ImVec2 ViewportSize   = Toolkit::Composer::GetItemRectSize();

        // Keep the multi-selection set consistent with single-selection changes made in Hierarchy or Inspector.
        mTools.GetEntities().ReconcileSelection();

        const Camera Camera(Director, ViewportOrigin, ViewportSize);

        // A light marker only reads against an image the light took part in, so it stays off the raw buffers.
        if (mTarget == Renderer::Target::Radiance || mTarget == Renderer::Target::Final)
        {
            DrawLightMarkers(Camera);
        }

        // Mark every selected entity with corner brackets, drawn as an overlay so they show under any brush.
        const auto DrawBrackets = [&](IntRect AABB)
        {
            const ImVec2 Corner[4] = {
                Camera.Project(Placement(0, 0, AABB.GetMinimumX(), AABB.GetMinimumY())),
                Camera.Project(Placement(0, 0, AABB.GetMaximumX(), AABB.GetMinimumY())),
                Camera.Project(Placement(0, 0, AABB.GetMaximumX(), AABB.GetMaximumY())),
                Camera.Project(Placement(0, 0, AABB.GetMinimumX(), AABB.GetMaximumY())),
            };

            constexpr UInt32 Color = Toolkit::Theme::kSelect;
            constexpr Real32 Thick = 2.0f;

            const Ptr<ImDrawList> List = Toolkit::Composer::GetWindowDrawList();

            // At each corner, draw a short arm toward each of its two neighbours along the quad edges.
            const auto DrawArm = [&](ImVec2 From, ImVec2 To)
            {
                const Real32 Length = Sqrt((To.x - From.x) * (To.x - From.x) + (To.y - From.y) * (To.y - From.y));
                const Real32 Reach = Length > 0.0001f ? Min(24.0f, Length * 0.35f) / Length : 0.0f;

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
            if (!Actor.IsValid())
            {
                return;
            }

            IntRect Enclosure = IntRect::Zero();

            if (CollectBounds(GetContext().GetDirector().GetProjection(), Actor, Enclosure, false))
            {
                DrawBrackets(Enclosure);
            }

            if (const ConstPtr<Tileon::Transform> Transform = Actor.TryGet<const Tileon::Transform>())
            {
                const Vector3 World = Transform->GetWorldspace().GetTranslation() + Vector3(Transform->GetOrigin());

                if (Abs(World.GetY()) > 0.0001f)
                {
                    DrawStem(Camera, World, Toolkit::Theme::kSelectSoft);
                }

                if (const ConstPtr<Tileon::Spotlight> Light = Actor.TryGet<const Tileon::Spotlight>())
                {
                    DrawSpotlightCone(Camera, * Transform, * Light);
                }

                if (const ConstPtr<Tileon::Glowlight> Light = Actor.TryGet<const Tileon::Glowlight>())
                {
                    DrawGlowlightArea(Camera, * Transform, * Light);
                }
            }
        };

        if (ConstRef<Bag<UInt64> > Selection = mTools.GetEntities().GetSelection(); !Selection.IsEmpty())
        {
            for (const UInt64 ID: Selection)
            {
                DrawSelected(GetContext().GetScene().GetEntity(ID));
            }
        }
        else if (const UInt64 Selected = GetContext().GetInteger("Selection.Entity", 0))
        {
            DrawSelected(GetContext().GetScene().GetEntity(Selected));
        }

        // Clipboard and delete shortcuts act on the whole selection whenever the viewport holds focus.
        if (Toolkit::Composer::IsWindowFocused())
        {
            const Bool Control = Toolkit::Composer::IsKeyDown(ImGuiMod_Ctrl);

            if (Control && Toolkit::Composer::IsKeyPressed(ImGuiKey_C))
            {
                mTools.GetEntities().CopySelection();
            }
            else if (Control && Toolkit::Composer::IsKeyPressed(ImGuiKey_X))
            {
                mTools.GetEntities().CutSelection();
            }

            if (Toolkit::Composer::IsKeyPressed(ImGuiKey_Delete))
            {
                mTools.GetEntities().DeleteSelection();
            }
        }

        Bool Manipulating = false;

        if (mTools.GetBrush() == Tools::Brush::Select)
        {
            if (Toolkit::Composer::IsWindowFocused())
            {
                if (Toolkit::Composer::IsKeyPressed(ImGuiKey_Q))
                {
                    mGizmo.SetMode(Gizmo::Mode::Move);
                }
                else if (Toolkit::Composer::IsKeyPressed(ImGuiKey_W))
                {
                    mGizmo.SetMode(Gizmo::Mode::Rotate);
                }
                else if (Toolkit::Composer::IsKeyPressed(ImGuiKey_E))
                {
                    mGizmo.SetMode(Gizmo::Mode::Scale);
                }
            }

            const UInt64        Selection = GetContext().GetInteger("Selection.Entity", 0);
            const Scene::Entity Actor     = GetContext().GetScene().GetEntity(Selection);

            // A marquee drag must own the cursor outright, so the handles stand down while one is in progress.
            if (!mMarquee)
            {
                Manipulating = mGizmo.Draw(mTools.GetEntities().GetSelection(), Actor, ViewportOrigin, ViewportSize);
            }
        }

        // Handle interactions with the viewport, such as hovering and clicking to manipulate the scene.
        // A drag that belongs to the handles must not also reach the brush, or picking would fight the gizmo.
        if (!Manipulating && Toolkit::Composer::IsItemHovered())
        {
            const UInt32 AbsoluteX  = Toolkit::Composer::GetMousePos().x - Toolkit::Composer::GetItemRectMin().x;
            const Real32 NormalizeX = AbsoluteX / Toolkit::Composer::GetItemRectSize().x;
            const UInt32 AbsoluteY  = Toolkit::Composer::GetMousePos().y - Toolkit::Composer::GetItemRectMin().y;
            const Real32 NormalizeY = AbsoluteY / Toolkit::Composer::GetItemRectSize().y;

            // Ctrl + wheel resizes the pending entity.
            if (const Real32 Wheel = Toolkit::Composer::GetMouseWheel(); Wheel != 0.0f)
            {
                if (mTools.GetEntities().HasPreview() && Toolkit::Composer::IsKeyDown(ImGuiMod_Ctrl))
                {
                    mTools.GetEntities().AdjustPreviewScale(Wheel);
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
            if (mTools.GetEntities().HasPreview())
            {
                const Real32 Direction =
                    (Toolkit::Composer::IsKeyDown(ImGuiKey_E) ? 1.0f : 0.0f) -
                    (Toolkit::Composer::IsKeyDown(ImGuiKey_Q) ? 1.0f : 0.0f);

                if (Direction != 0.0f)
                {
                    const Real32 Speed = Toolkit::Composer::IsKeyDown(ImGuiMod_Shift) ? 30.0f : 120.0f;
                    mTools.GetEntities().AdjustPreviewRotation(Angle::FromDegrees(Direction * Speed * Toolkit::Composer::GetDeltaTime()));
                }
            }

            // Converts this frame's cursor pixel delta into a world-space shift of the camera.
            const auto PanByCursorDelta = [&]()
            {
                const ImVec2 Delta = Toolkit::Composer::GetMouseDelta();
                const ImVec2 Size  = Toolkit::Composer::GetItemRectSize();

                const Real32 ScaleX = Director.GetViewport().GetX() * Director.GetDensity() / Size.x;
                const Real32 ScaleY = Director.GetViewport().GetY() * Director.GetDensity() / Size.y;

                const Vector2 OldPosition(AbsoluteX * ScaleX, AbsoluteY* ScaleY);
                const Vector2 NewPosition((AbsoluteX - Delta.x) * ScaleX, (AbsoluteY - Delta.y) * ScaleY);

                const Placement OldPlacement = Director.GetWorldCoordinates(OldPosition);
                const Placement NewPlacement = Director.GetWorldCoordinates(NewPosition);
                Director.SetPosition(Placement::Normalize(Director.GetPosition() + NewPlacement - OldPlacement));
            };

            // Alt and a drag swing the free view around, which is the whole reason it exists: a 3D shape read
            // straight on is a rectangle, and a few degrees of yaw is what tells its depth from its height.
            const Perspective Viewing  = GetContext().GetEnum("Viewport.Perspective", Perspective::Ortho);
            const Bool        Swinging = Viewing == Perspective::Axonometric
                                      && Toolkit::Composer::IsKeyDown(ImGuiMod_Alt)
                                      && Toolkit::Composer::IsMouseDown(ImGuiMouseButton_Left);

            // Hold Space (or drag with the middle mouse) to pan the view with any brush active, so the camera can
            // be repositioned mid-paint without switching to the hand tool.
            const Bool SpacePan = !Toolkit::Composer::IsTextInputActive() && Toolkit::Composer::IsKeyDown(ImGuiKey_Space);
            const Bool Panning  = Toolkit::Composer::IsMouseDragging(ImGuiMouseButton_Middle)
                               || (SpacePan && Toolkit::Composer::IsMouseDown(ImGuiMouseButton_Left));

            // Hint the pan affordance so holding Space is discoverable.
            if (SpacePan)
            {
                Toolkit::Composer::SetMouseCursor(ImGuiMouseCursor_Hand);
            }

            if (Swinging)
            {
                const ImVec2 Swing = Toolkit::Composer::GetMouseDelta();

                mYaw  = Mod(mYaw + Swing.x * 0.4f, 360.0f);
                mTilt = Clamp(mTilt - Swing.y * 0.004f, kMinTilt, kMaxTilt);

                Director.SetProjection(Tileon::Projection::Axonometric(Angle::FromDegrees(mYaw), mTilt));

                mTools.GetEntities().ClearPreview();
                mMarquee      = false;
                mMarqueeMoved = false;
            }
            else if (Panning)
            {
                PanByCursorDelta();
            }
            else if (mTools.GetBrush() == Tools::Brush::Hand)
            {
                mTools.GetEntities().ClearPreview();

                if (Toolkit::Composer::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    PanByCursorDelta();
                }
            }
            else if (mTools.GetBrush() == Tools::Brush::Select)
            {
                mTools.GetEntities().ClearPreview();

                const Placement Cursor = Director.GetWorldCoordinates(Vector2(AbsoluteX, AbsoluteY));
                const Bool      Shift  = Toolkit::Composer::IsKeyDown(ImGuiMod_Shift);

                // Paste the clipboard's group so its anchor lands on the cursor.
                if (Toolkit::Composer::IsKeyDown(ImGuiMod_Ctrl) && Toolkit::Composer::IsKeyPressed(ImGuiKey_V))
                {
                    mTools.GetEntities().Paste(Cursor);
                }

                // Outline what a click would take. A marquee decides by area instead, so it takes over.
                if (!mMarquee)
                {
                    DrawSelectionHint(Camera, mTools.GetEntities().ResolveSelection(Cursor));
                }

                // A press starts either a marquee or a plain click; the drag distance decides which on release.
                if (Toolkit::Composer::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    mMarquee       = true;
                    mMarqueeMoved  = false;
                    mMarqueeScreen = Toolkit::Composer::GetMousePos();
                }

                if (mMarquee && Toolkit::Composer::IsMouseDown(ImGuiMouseButton_Left))
                {
                    const ImVec2 Now = Toolkit::Composer::GetMousePos();

                    if (Abs(Now.x - mMarqueeScreen.x) + Abs(Now.y - mMarqueeScreen.y) > 4.0f)
                    {
                        mMarqueeMoved = true;
                    }

                    if (mMarqueeMoved)
                    {
                        const ImVec2 Lower(Min(mMarqueeScreen.x, Now.x), Min(mMarqueeScreen.y, Now.y));
                        const ImVec2 Upper(Max(mMarqueeScreen.x, Now.x), Max(mMarqueeScreen.y, Now.y));

                        const Ptr<ImDrawList> List = Toolkit::Composer::GetWindowDrawList();
                        List->AddRectFilled(Lower, Upper, Toolkit::Theme::kSelectWash);
                        List->AddRect(Lower, Upper, Toolkit::Theme::kSelect);
                    }
                }

                if (mMarquee && Toolkit::Composer::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    mMarquee = false;

                    if (mMarqueeMoved)
                    {
                        const ImVec2 Release = Toolkit::Composer::GetMousePos();
                        const ImVec2 Lower(Min(mMarqueeScreen.x, Release.x), Min(mMarqueeScreen.y, Release.y));
                        const ImVec2 Upper(Max(mMarqueeScreen.x, Release.x), Max(mMarqueeScreen.y, Release.y));

                        mTools.GetEntities().SelectWithin(Camera, Lower, Upper, Shift);
                    }
                    else if (Shift)
                    {
                        mTools.GetEntities().SelectToggle(Cursor);
                    }
                    else
                    {
                        mTools.GetEntities().SelectSingle(Cursor);
                    }
                }
            }
            else if (mTools.GetMode() == Tools::Mode::Ground)
            {
                mTools.GetEntities().ClearPreview();

                // the selection is taken as it stands rather than reserving a value to mean nothing.
                const UInt32    Selection = GetContext().GetInteger("Selection.Terrain", 0);
                const Placement Cursor    = Director.GetWorldCoordinates(Vector2(AbsoluteX, AbsoluteY));

                // Pencil paints a continuous stroke while held; Bucket fills once per click.
                const Bool   Continuous = (mTools.GetBrush() == Tools::Brush::Pencil);
                const SInt32 UnitX      = static_cast<SInt32>(Floor(Cursor.GetAbsoluteX()));
                const SInt32 UnitY      = static_cast<SInt32>(Floor(Cursor.GetAbsoluteY()));

                // Ground is worked in rather than stamped, so holding the brush still keeps depositing and the
                // terrain comes on the longer it is leant on.
                const Real64 Now     = Toolkit::Composer::GetTime();
                const Bool   Moved   = (UnitX != mPaintUnitX) || (UnitY != mPaintUnitY);
                const Bool   NewUnit = Moved || (Now - mPaintTime) >= Ground::kCadence;

                const Bool LeftClick  = Toolkit::Composer::IsMouseClicked(ImGuiMouseButton_Left);
                const Bool RightClick = Toolkit::Composer::IsMouseClicked(ImGuiMouseButton_Right);
                const Bool LeftHeld   = Toolkit::Composer::IsMouseDown(ImGuiMouseButton_Left);
                const Bool RightHeld  = Toolkit::Composer::IsMouseDown(ImGuiMouseButton_Right);

                const Bool Erase = RightClick || (Continuous && RightHeld && NewUnit);
                const Bool Paint = LeftClick  || (Continuous && LeftHeld  && NewUnit);

                // A stroke held across many units is one edit, so it keeps a single step open until it is let go.
                if ((LeftHeld || RightHeld) && !mStroke)
                {
                    mStroke = true;

                    GetContext().GetHistory().Open("Paint Ground"_Text);
                }

                if (Erase)
                {
                    mTools.Execute(Tools::Command::Remove, Cursor, Selection);
                }
                else if (Paint)
                {
                    mTools.Execute(Tools::Command::Add, Cursor, Selection);
                }

                if (LeftHeld || RightHeld)
                {
                    mPaintUnitX = UnitX;
                    mPaintUnitY = UnitY;

                    if (NewUnit)
                    {
                        mPaintTime = Now;
                    }
                }
            }
            else
            {
                const Bool IsLeftButton  = Toolkit::Composer::IsMouseClicked(ImGuiMouseButton_Left);
                const Bool IsRightButton = Toolkit::Composer::IsMouseClicked(ImGuiMouseButton_Right);

                const UInt32 Selection = GetContext().GetInteger("Selection.Archetype", 0);

                Placement Cursor = Director.GetWorldCoordinates(Vector2(AbsoluteX, AbsoluteY));

                // Snap to the center of the hovered tile when the toggle is on, or momentarily while Shift is held.
                if (mTools.IsAligned() || Toolkit::Composer::IsKeyDown(ImGuiMod_Shift))
                {
                    Cursor = Placement::FromAbsolute(
                        Floor(Cursor.GetAbsoluteX()) + 0.5,
                        Floor(Cursor.GetAbsoluteY()) + 0.5);
                }

                // Show the pending entity under the cursor before it is committed by a click.
                mTools.GetEntities().UpdatePreview(mTools.GetBrush(), Cursor, Selection);

                // Handle left-click for adding and right-click for removing entities.
                if (IsRightButton || (IsLeftButton && Selection != 0))
                {
                    const Tools::Command Command = IsLeftButton
                        ? Tools::Command::Add
                        : Tools::Command::Remove;
                    mTools.Execute(Command, Cursor, Selection);
                }
            }
        }
        else
        {
            mTools.GetEntities().ClearPreview();
        }
    }
}