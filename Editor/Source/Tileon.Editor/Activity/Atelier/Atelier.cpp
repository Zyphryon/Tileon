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

    Atelier::Atelier(Ref<Context> Context)
        : Activity      { Context, kTitle, true  },
          mWorkshop     { Context },
          mGizmo        { Context },
          mTarget       { Renderer::Target::Albedo },
          mTimescale    { 1.0f },
          mYaw          { 0.0f },
          mTilt         { kMaxTilt },
          mMarquee      { false },
          mMarqueeMoved { false },
          mPaintTileX   { INT32_MIN },
          mPaintTileY   { INT32_MIN }
    {
        // The preview phase resolves one target, so it starts on the one the viewport opens with.
        Context.GetRenderer().SetOutput(mTarget);

        // A light draws nothing of its own, so the viewport marks where each one sits.
        Ref<Scene::Service> Scene = Context.GetScene();

        mQrGlowlights = Scene.CreateQuery<
            Scene::DSL::With<Tileon::Glowlight>, Scene::DSL::In<const Tileon::Transform, ConstPtr<IntColor8>>
        >("Editor::Atelier::Glowlights", Scene::Cache::Auto);

        mQrSpotlights = Scene.CreateQuery<
            Scene::DSL::With<Tileon::Spotlight>, Scene::DSL::In<const Tileon::Transform, ConstPtr<IntColor8>>
        >("Editor::Atelier::Spotlights", Scene::Cache::Auto);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::OnDraw()
    {
        mWorkshop.Tick();

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
            mWorkshop.ClearPreview();
        }
        Toolkit::Composer::End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawToolbar()
    {
        // Draw mode switch button to toggle between tile editing and entity editing modes.
        switch (mWorkshop.GetMode())
        {
        case Workshop::Mode::Tile:
            if (Toolkit::Composer::Button(ICON_FA_MAP "##mode", 32.0f))
            {
                mWorkshop.SetBrush(Workshop::Brush::Pencil);
                mWorkshop.SetMode(Workshop::Mode::Entity);
            }
            Toolkit::Composer::Tooltip("Switch to entity mode");
            break;
        case Workshop::Mode::Entity:
            if (Toolkit::Composer::Button(ICON_FA_CUBE "##mode", 32.0f))
            {
                mWorkshop.SetBrush(Workshop::Brush::Pencil);
                mWorkshop.SetMode(Workshop::Mode::Tile);
            }
            Toolkit::Composer::Tooltip("Switch to tile mode");
            break;
        }
        Toolkit::Composer::SameLine();

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

        switch (mWorkshop.GetMode())
        {
        case Workshop::Mode::Tile:
            DrawTileToolbar();
            break;
        case Workshop::Mode::Entity:
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

        Perspective CurrentMode = GetContext().GetEnum("Atelier.Perspective", Perspective::Ortho);

        if (Toolkit::Composer::Combo("##projection", CurrentMode))
        {
            GetContext().SetEnum("Atelier.Perspective", CurrentMode);

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

    void Atelier::DrawBrushButton(Workshop::Brush Brush, Text Icon, Text Tooltip)
    {
        const Bool Active = (mWorkshop.GetBrush() == Brush);

        if (Active)
        {
            Toolkit::Composer::PushStyleColor(ImGuiCol_Button,        Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
            Toolkit::Composer::PushStyleColor(ImGuiCol_ButtonHovered, Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        if (Toolkit::Composer::Button(String<64>::Print<"{0}##{1}">(Icon, Enum::GetName(Brush)), 32.0f))
        {
            mWorkshop.SetBrush(Brush);
        }

        Toolkit::Composer::Tooltip(Tooltip);

        if (Active)
        {
            Toolkit::Composer::PopStyleColor(2);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawSelectionHint(ConstRef<Lens> Lens, Scene::Entity Actor)
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
            return Lens.Project(Placement(0, 0, X, Z), static_cast<Real32>(Y));
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

        constexpr UInt32 kTint = Toolkit::Palette::kHint;

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

    void Atelier::DrawTimescaleToolbar()
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

    void Atelier::DrawDebugButton(Renderer::Debug Overlay, Text Icon, Text Tooltip)
    {
        const Bool Active = GetContext().GetRenderer().HasProperty(Overlay);

        if (Active)
        {
            Toolkit::Composer::PushStyleColor(ImGuiCol_Button,        Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
            Toolkit::Composer::PushStyleColor(ImGuiCol_ButtonHovered, Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        if (Toolkit::Composer::Button(String<64>::Print<"{0}##{1}">(Icon, Enum::GetName(Overlay)), 32.0f))
        {
            GetContext().GetRenderer().SetProperty(Overlay, !Active);
        }

        Toolkit::Composer::Tooltip(Tooltip);

        if (Active)
        {
            Toolkit::Composer::PopStyleColor(2);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawTileToolbar()
    {
        DrawBrushButton(Workshop::Brush::Hand,   ICON_FA_HAND,          "Pan the view");
        Toolkit::Composer::SameLine();

        DrawBrushButton(Workshop::Brush::Select, ICON_FA_ARROW_POINTER, "Select");
        Toolkit::Composer::SameLine();

        DrawBrushButton(Workshop::Brush::Pencil, ICON_FA_BRUSH,         "Paint tiles");
        Toolkit::Composer::SameLine();

        DrawBrushButton(Workshop::Brush::Bucket, ICON_FA_FILL_DRIP,     "Fill the region");
        Toolkit::Composer::SameLine();

        Toolkit::Composer::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        Toolkit::Composer::SameLine();

        // Draw the layer selection buttons for switching between layers.
        const auto DrawLayerButton = [&](Workshop::Level Level, Text Icon, Text Hint)
        {
            const Bool Active = (mWorkshop.GetLevel() == Level);

            if (Active)
            {
                Toolkit::Composer::PushStyleColor(ImGuiCol_Button,        Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
                Toolkit::Composer::PushStyleColor(ImGuiCol_ButtonHovered, Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
            }

            if (Toolkit::Composer::Button(String<64>::Print<"{0}##{1}">(Icon, Enum::GetName(Level)), 32.0f))
            {
                mWorkshop.SetLevel(Level);
            }

            Toolkit::Composer::Tooltip(Hint);

            if (Active)
            {
                Toolkit::Composer::PopStyleColor(2);
            }
        };

        Toolkit::Composer::Label(ICON_FA_LAYER_GROUP);
        Toolkit::Composer::SameLine();

        DrawLayerButton(Workshop::Level::Base,   ICON_FA_1, "Base layer");
        Toolkit::Composer::SameLine();

        DrawLayerButton(Workshop::Level::Detail, ICON_FA_2, "Detail layer");
        Toolkit::Composer::SameLine();

        Toolkit::Composer::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        Toolkit::Composer::SameLine();

        const Bool Anchored = (mWorkshop.GetAlignment() == Workshop::Alignment::Cursor);

        if (Anchored)
        {
            Toolkit::Composer::PushStyleColor(ImGuiCol_Button,        Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
            Toolkit::Composer::PushStyleColor(ImGuiCol_ButtonHovered, Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        if (Toolkit::Composer::Button(ICON_FA_CROSSHAIRS "##Alignment", 32.0f))
        {
            mWorkshop.SetAlignment(Anchored ? Workshop::Alignment::Lattice : Workshop::Alignment::Cursor);
        }

        if (Anchored)
        {
            Toolkit::Composer::Tooltip("Anchor the art where it is painted");
        }
        else
        {
            Toolkit::Composer::Tooltip("Follow the lattice, keeping neighbouring paint seamless");
        }

        if (Anchored)
        {
            Toolkit::Composer::PopStyleColor(2);
        }

        Toolkit::Composer::SameLine();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawEntityToolbar()
    {
        // Entities are placed one at a time, so the area-filling bucket has no meaning here.
        DrawBrushButton(Workshop::Brush::Hand,   ICON_FA_HAND,          "Pan the view");
        Toolkit::Composer::SameLine();

        DrawBrushButton(Workshop::Brush::Select, ICON_FA_ARROW_POINTER, "Select");
        Toolkit::Composer::SameLine();

        DrawBrushButton(Workshop::Brush::Pencil, ICON_FA_BRUSH,         "Place entity");
        Toolkit::Composer::SameLine();

        Toolkit::Composer::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        Toolkit::Composer::SameLine();

        // Snap placement to tile centers, so entities line up on the grid instead of landing at the exact cursor.
        const Bool Aligned = mWorkshop.IsAligned();

        if (Aligned)
        {
            Toolkit::Composer::PushStyleColor(ImGuiCol_Button,        Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
            Toolkit::Composer::PushStyleColor(ImGuiCol_ButtonHovered, Toolkit::Composer::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        if (Toolkit::Composer::Button(ICON_FA_CROSSHAIRS "##center", 32.0f))
        {
            mWorkshop.SetAligned(!Aligned);
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

    void Atelier::DrawLightMarkers(ConstRef<Lens> Lens)
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
            return Tint ? IM_COL32(Tint->GetRed(), Tint->GetGreen(), Tint->GetBlue(), 235) : Toolkit::Palette::kMarker;
        };

        mQrGlowlights.Run([&](ConstRef<Tileon::Transform> Transform, ConstPtr<IntColor8> Tint)
        {
            const Vector3 World = Locate(Transform);
            const ImVec2  At    = Lens.Project(Placement::FromAbsolute(World.GetX(), World.GetZ()), World.GetY());

            DrawStem(Lens, World, Toolkit::Palette::kMarkerFaint);
            DrawIcon(At, ICON_FA_LIGHTBULB, Shade(Tint));
        });

        mQrSpotlights.Run([&](ConstRef<Tileon::Transform> Transform, ConstPtr<IntColor8> Tint)
        {
            const Vector3 World = Locate(Transform);
            const ImVec2  At    = Lens.Project(Placement::FromAbsolute(World.GetX(), World.GetZ()), World.GetY());

            DrawStem(Lens, World, Toolkit::Palette::kMarkerFaint);
            DrawIcon(At, ICON_FA_FILTER, Shade(Tint));
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawWorldRing(ConstRef<Lens> Lens, Vector3 Center, Vector3 AxisU, Vector3 AxisV, Real32 Radius, UInt32 Color)
    {
        constexpr UInt32 kSegments = 24;

        ImVec2 Ring[kSegments];

        for (UInt32 Step = 0; Step < kSegments; ++Step)
        {
            const Real32  Theta = (2.0f * kPI<Real32> * Step) / kSegments;
            const Vector3 Point = Center + (AxisU * Base::Cosine(Theta) + AxisV * Base::Sine(Theta)) * Radius;

            Ring[Step] = Lens.Project(Placement::FromAbsolute(Point.GetX(), Point.GetZ()), Point.GetY());
        }

        Toolkit::Composer::GetWindowDrawList()->AddPolyline(Ring, kSegments, Color, ImDrawFlags_Closed, 1.5f);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawSpotlightCone(ConstRef<Lens> Lens, ConstRef<Tileon::Transform> Transform, ConstRef<Tileon::Spotlight> Light)
    {
        constexpr UInt32 kColor = Toolkit::Palette::kSelectSoft;

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

        DrawWorldRing(Lens, Center, Right, Up, Radius, kColor);

        // Four edges carry the spread without the mush eight would make at a shallow tilt.
        const Vector3 Rim[4] =
        {
            Center + Right * Radius, Center + Up * Radius, Center - Right * Radius, Center - Up * Radius,
        };

        const Ptr<ImDrawList> List = Toolkit::Composer::GetWindowDrawList();
        const ImVec2          Apex = Lens.Project(Placement::FromAbsolute(World.GetX(), World.GetZ()), World.GetY());

        for (UInt32 Edge = 0; Edge < 4; ++Edge)
        {
            const ImVec2 Tip = Lens.Project(Placement::FromAbsolute(Rim[Edge].GetX(), Rim[Edge].GetZ()), Rim[Edge].GetY());

            List->AddLine(Apex, Tip, kColor, 1.5f);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawGlowlightArea(ConstRef<Lens> Lens, ConstRef<Tileon::Transform> Transform, ConstRef<Tileon::Glowlight> Light)
    {
        const Vector3 World = Transform.GetWorldspace().GetTranslation() + Vector3(Transform.GetOrigin());
        const Vector3 Scale = Transform.GetWorldspace().GetScale();

        const Real32 Radius = Light.GetRadius() * Max(Scale.GetX(), Max(Scale.GetY(), Scale.GetZ()));

        constexpr Vector3 AxisX = Vector3::UnitX();
        constexpr Vector3 AxisY = Vector3::UnitY();
        constexpr Vector3 AxisZ = Vector3::UnitZ();

        DrawWorldRing(Lens, World, AxisX, AxisZ, Radius, Toolkit::Palette::kSelectSoft);
        DrawWorldRing(Lens, World, AxisX, AxisY, Radius, Toolkit::Palette::kSelectSoft);
        DrawWorldRing(Lens, World, AxisY, AxisZ, Radius, Toolkit::Palette::kSelectSoft);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Atelier::DrawStem(ConstRef<Lens> Lens, Vector3 World, UInt32 Color)
    {
        constexpr Real32 kDash     = 2.0f;
        constexpr Real32 kStride   = 5.0f;
        constexpr Real32 kRadius   = 5.0f;
        constexpr UInt32 kSegments = 20;

        const Ptr<ImDrawList> List   = Toolkit::Composer::GetWindowDrawList();
        const Placement       Ground = Placement::FromAbsolute(World.GetX(), World.GetZ());
        const ImVec2          At     = Lens.Project(Ground, World.GetY());
        const ImVec2          Foot   = Lens.Project(Ground, 0.0f);

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

        const ImVec2 AxisX = Lens.Direction(Vector3(1.0f, 0.0f, 0.0f));
        const ImVec2 AxisZ = Lens.Direction(Vector3(0.0f, 0.0f, 1.0f));
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

    void Atelier::DrawViewport()
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
        mWorkshop.ReconcileSelection();

        const Lens Lens(Director, ViewportOrigin, ViewportSize);

        // A light marker only reads against an image the light took part in, so it stays off the raw buffers.
        if (mTarget == Renderer::Target::Radiance || mTarget == Renderer::Target::Final)
        {
            DrawLightMarkers(Lens);
        }

        // Mark every selected entity with corner brackets, drawn as an overlay so they show under any brush.
        const auto DrawBrackets = [&](IntRect AABB)
        {
            const ImVec2 Corner[4] = {
                Lens.Project(Placement(0, 0, AABB.GetMinimumX(), AABB.GetMinimumY())),
                Lens.Project(Placement(0, 0, AABB.GetMaximumX(), AABB.GetMinimumY())),
                Lens.Project(Placement(0, 0, AABB.GetMaximumX(), AABB.GetMaximumY())),
                Lens.Project(Placement(0, 0, AABB.GetMinimumX(), AABB.GetMaximumY())),
            };

            constexpr UInt32 Color = Toolkit::Palette::kSelect;
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
                    DrawStem(Lens, World, Toolkit::Palette::kSelectSoft);
                }

                if (const ConstPtr<Tileon::Spotlight> Light = Actor.TryGet<const Tileon::Spotlight>())
                {
                    DrawSpotlightCone(Lens, * Transform, * Light);
                }

                if (const ConstPtr<Tileon::Glowlight> Light = Actor.TryGet<const Tileon::Glowlight>())
                {
                    DrawGlowlightArea(Lens, * Transform, * Light);
                }
            }
        };

        if (ConstRef<Bag<UInt64> > Selection = mWorkshop.GetSelection(); !Selection.IsEmpty())
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
                mWorkshop.CopySelection();
            }
            else if (Control && Toolkit::Composer::IsKeyPressed(ImGuiKey_X))
            {
                mWorkshop.CutSelection();
            }

            if (Toolkit::Composer::IsKeyPressed(ImGuiKey_Delete))
            {
                mWorkshop.DeleteSelection();
            }
        }

        Bool Manipulating = false;

        if (mWorkshop.GetBrush() == Workshop::Brush::Select)
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
                Manipulating = mGizmo.Draw(mWorkshop.GetSelection(), Actor, ViewportOrigin, ViewportSize);
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
                if (mWorkshop.HasPreview() && Toolkit::Composer::IsKeyDown(ImGuiMod_Ctrl))
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
                    (Toolkit::Composer::IsKeyDown(ImGuiKey_E) ? 1.0f : 0.0f) -
                    (Toolkit::Composer::IsKeyDown(ImGuiKey_Q) ? 1.0f : 0.0f);

                if (Direction != 0.0f)
                {
                    const Real32 Speed = Toolkit::Composer::IsKeyDown(ImGuiMod_Shift) ? 30.0f : 120.0f;
                    mWorkshop.AdjustPreviewRotation(Angle::FromDegrees(Direction * Speed * Toolkit::Composer::GetDeltaTime()));
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
            const Perspective Viewing  = GetContext().GetEnum("Atelier.Perspective", Perspective::Ortho);
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

                mWorkshop.ClearPreview();
                mMarquee      = false;
                mMarqueeMoved = false;
            }
            else if (Panning)
            {
                PanByCursorDelta();
            }
            else if (mWorkshop.GetBrush() == Workshop::Brush::Hand)
            {
                mWorkshop.ClearPreview();

                if (Toolkit::Composer::IsMouseDragging(ImGuiMouseButton_Left))
                {
                    PanByCursorDelta();
                }
            }
            else if (mWorkshop.GetBrush() == Workshop::Brush::Select)
            {
                mWorkshop.ClearPreview();

                const Placement Cursor = Director.GetWorldCoordinates(Vector2(AbsoluteX, AbsoluteY));
                const Bool      Shift  = Toolkit::Composer::IsKeyDown(ImGuiMod_Shift);

                // Paste the clipboard's group so its anchor lands on the cursor.
                if (Toolkit::Composer::IsKeyDown(ImGuiMod_Ctrl) && Toolkit::Composer::IsKeyPressed(ImGuiKey_V))
                {
                    mWorkshop.Paste(Cursor);
                }

                // Outline what a click would take. A marquee decides by area instead, so it takes over.
                if (!mMarquee)
                {
                    DrawSelectionHint(Lens, mWorkshop.ResolveSelection(Cursor));
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
                        List->AddRectFilled(Lower, Upper, Toolkit::Palette::kSelectWash);
                        List->AddRect(Lower, Upper, Toolkit::Palette::kSelect);
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
                    const Bool       IsBucket = (mWorkshop.GetBrush() == Workshop::Brush::Bucket);
                    const IntVector2 Span     = Selection ? mContext.GetTileset().GetMotif(Selection).GetPeriod() : IntVector2::One();

                    IntRect Footprint;

                    if (IsBucket)
                    {
                        Footprint = IntRect(
                            Cursor.GetBaseX(), Cursor.GetBaseY(),
                            Cursor.GetBaseX() + Tileon::Region::kTilesPerX,
                            Cursor.GetBaseY() + Tileon::Region::kTilesPerY);
                    }
                    else
                    {
                        const SInt32 TileX = static_cast<SInt32>(Floor(Cursor.GetAbsoluteX()));
                        const SInt32 TileY = static_cast<SInt32>(Floor(Cursor.GetAbsoluteY()));

                        Footprint = IntRect(TileX, TileY, TileX + Span.GetX(), TileY + Span.GetY());
                    }

                    const auto Project = [&](SInt32 X, SInt32 Y)
                    {
                        return Lens.Project(Placement(0, 0, X, Y));
                    };

                    const ImVec2 Corner0 = Project(Footprint.GetMinimumX(), Footprint.GetMinimumY());
                    const ImVec2 Corner1 = Project(Footprint.GetMaximumX(), Footprint.GetMinimumY());
                    const ImVec2 Corner2 = Project(Footprint.GetMaximumX(), Footprint.GetMaximumY());
                    const ImVec2 Corner3 = Project(Footprint.GetMinimumX(), Footprint.GetMaximumY());

                    const Ptr<ImDrawList> List = Toolkit::Composer::GetWindowDrawList();

                    ConstRef<Tileset::Glyph> Glyph = mContext.GetTileset().GetGlyph(Selection);

                    if (Selection != 0 && !IsBucket && Glyph.Texture)
                    {
                        // Every slice shares one identifier, so which of them is drawn rides in the coordinates.
                        const ImTextureID Slice  = Plugin::ImGuiRenderer::GetLayeredTextureID(Glyph.Texture);
                        const auto        Sample = [&](Real32 U, Real32 V)
                        {
                            return Plugin::ImGuiRenderer::GetLayeredTextureUV(Glyph.Slice, ImVec2(U, V));
                        };

                        const UInt32 Base    = Glyph.Tint.ToRGBA8();
                        const UInt32 Alpha   = static_cast<UInt32>(((Base >> 24) & 0xFF) * 0.7f) << 24;
                        const UInt32 Preview = (Base & 0x00FFFFFF) | Alpha;

                        const IntVector2 Period(Max(Span.GetX(), 1), Max(Span.GetY(), 1));
                        const SInt32     SpanX = Period.GetX();
                        const SInt32     SpanY = Period.GetY();

                        // Atlas region of one sub-tile within the motif, which owns its slice whole.
                        const Real32 CellU = 1.0f / SpanX;
                        const Real32 CellV = 1.0f / SpanY;

                        const IntVector2 Origin(Footprint.GetMinimumX(), Footprint.GetMinimumY());
                        const IntVector2 Anchor = IntVector2(mWorkshop.Resolve(Origin, Period));

                        // Draw the motif cell-by-cell so each cell shows the exact sub-tile the paint will store.
                        for (SInt32 CellY = Footprint.GetMinimumY(); CellY < Footprint.GetMaximumY(); ++CellY)
                        {
                            for (SInt32 CellX = Footprint.GetMinimumX(); CellX < Footprint.GetMaximumX(); ++CellX)
                            {
                                const IntVector2 Phase  = Tile::Align(IntVector2(CellX, CellY) - Anchor, Period);
                                const SInt32     Column = Phase.GetX();
                                const SInt32     Row    = Phase.GetY();

                                const Real32 U0 = Column            * CellU;
                                const Real32 V0 = (SpanY - 1 - Row) * CellV;

                                List->AddImageQuad(Slice,
                                    Project(CellX,     CellY),     Project(CellX + 1, CellY),
                                    Project(CellX + 1, CellY + 1), Project(CellX,     CellY + 1),
                                    Sample(U0, V0 + CellV), Sample(U0 + CellU, V0 + CellV),
                                    Sample(U0 + CellU, V0), Sample(U0, V0),
                                    Preview);
                            }
                        }
                    }
                    else
                    {
                        List->AddQuadFilled(Corner0, Corner1, Corner2, Corner3, Toolkit::Palette::kBrushWash);
                    }

                    // Outline always, so the footprint boundary stays legible over any art.
                    List->AddQuad(Corner0, Corner1, Corner2, Corner3, Toolkit::Palette::kBrush, 1.5f);
                }

                // Pencil paints a continuous stroke while held; Bucket fills once per click.
                const Bool   Continuous = (mWorkshop.GetBrush() == Workshop::Brush::Pencil);
                const SInt32 TileX      = static_cast<SInt32>(Floor(Cursor.GetAbsoluteX()));
                const SInt32 TileY      = static_cast<SInt32>(Floor(Cursor.GetAbsoluteY()));
                const Bool   NewTile    = (TileX != mPaintTileX) || (TileY != mPaintTileY);

                const Bool LeftClick  = Toolkit::Composer::IsMouseClicked(ImGuiMouseButton_Left);
                const Bool RightClick = Toolkit::Composer::IsMouseClicked(ImGuiMouseButton_Right);
                const Bool LeftHeld   = Toolkit::Composer::IsMouseDown(ImGuiMouseButton_Left);
                const Bool RightHeld  = Toolkit::Composer::IsMouseDown(ImGuiMouseButton_Right);

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
                const Bool IsLeftButton  = Toolkit::Composer::IsMouseClicked(ImGuiMouseButton_Left);
                const Bool IsRightButton = Toolkit::Composer::IsMouseClicked(ImGuiMouseButton_Right);

                const UInt32 Selection = GetContext().GetInteger("Selection.Archetype", 0);

                Placement Cursor = Director.GetWorldCoordinates(Vector2(AbsoluteX, AbsoluteY));

                // Snap to the center of the hovered tile when the toggle is on, or momentarily while Shift is held.
                if (mWorkshop.IsAligned() || Toolkit::Composer::IsKeyDown(ImGuiMod_Shift))
                {
                    Cursor = Placement::FromAbsolute(
                        Floor(Cursor.GetAbsoluteX()) + 0.5,
                        Floor(Cursor.GetAbsoluteY()) + 0.5);
                }

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