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

#include "Gizmo.hpp"
#include "Tileon.World/Component/State/Lifecycle.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Real32 Distance(ImVec2 First, ImVec2 Second)
    {
        const Real32 DeltaX = First.x - Second.x;
        const Real32 DeltaY = First.y - Second.y;
        return Sqrt(DeltaX * DeltaX + DeltaY * DeltaY);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Real32 DistanceToSegment(ImVec2 Point, ImVec2 Start, ImVec2 Finish)
    {
        const ImVec2 Span(Finish.x - Start.x, Finish.y - Start.y);
        const Real32 Length = Span.x * Span.x + Span.y * Span.y;

        if (Length < 0.0001f)
        {
            return Distance(Point, Start);
        }
        const Real32 Ratio = Clamp(((Point.x - Start.x) * Span.x + (Point.y - Start.y) * Span.y) / Length, 0.0f, 1.0f);
        const ImVec2 Closest(Start.x + Span.x * Ratio, Start.y + Span.y * Ratio);
        return Distance(Point, Closest);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void DrawArrow(Ptr<ImDrawList> List, ImVec2 Start, ImVec2 Finish, UInt32 Color)
    {
        const Real32 Length = Distance(Start, Finish);

        if (Length < 0.0001f)
        {
            return;
        }

        const ImVec2 Direction((Finish.x - Start.x) / Length, (Finish.y - Start.y) / Length);
        const ImVec2 Normal(-Direction.y, Direction.x);

        constexpr Real32 kHeadLength = 12.0f;
        constexpr Real32 kHeadWidth = 5.0f;

        const ImVec2 Base(Finish.x - Direction.x * kHeadLength, Finish.y - Direction.y * kHeadLength);

        List->AddLine(Start, Base, Color, 2.0f);
        List->AddTriangleFilled(
            Finish,
            ImVec2(Base.x + Normal.x * kHeadWidth, Base.y + Normal.y * kHeadWidth),
            ImVec2(Base.x - Normal.x * kHeadWidth, Base.y - Normal.y * kHeadWidth),
            Color);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Gizmo::Gizmo(Ref<Context> Context)
        : mContext { Context },
          mMode    { Mode::Move },
          mHandle  { Handle::None },
          mReach   { 0.0f }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Gizmo::Draw(Ref<UI::Composer> Composer, Scene::Entity Actor, ImVec2 Origin, ImVec2 Size)
    {
        // Always manipulate the group as a whole: whether the selection is the instance root or one of its parts,
        // transform the root, whose Pose cascades down to every part through World::ComputeWorldspace.
        Actor = Actor.IsValid() ? Scene::Entity::ResolveRecursively(Actor, Scene::Hierarchy::Fixed) : Actor;

        if (!Actor.IsValid() || !Actor.Has<Pose>())
        {
            mHandle = Handle::None;
            return false;
        }

        const ImVec2 Anchor = ToScreen(GetOrigin(Actor), Origin, Size);

        Handle Hovered = Handle::None;

        switch (mMode)
        {
        case Mode::Move:
            Hovered = DrawMove(Composer, Anchor);
            break;
        case Mode::Rotate:
            Hovered = DrawRotate(Composer, Anchor);
            break;
        case Mode::Scale:
            Hovered = DrawScale(Composer, Anchor);
            break;
        }

        if (mHandle == Handle::None)
        {
            if (Hovered != Handle::None && Composer.IsMouseClicked(ImGuiMouseButton_Left))
            {
                mHandle  = Hovered;
                mGrab    = ToWorld(Composer.GetMousePos(), Origin, Size);
                mStart   = GetOrigin(Actor);
                mOrigin  = Actor.Get<const Pose>();
                mReach   = Max(Distance(Composer.GetMousePos(), Anchor), 1.0f);
                mBearing = ImVec2(
                    (Composer.GetMousePos().x - Anchor.x) / mReach, (Composer.GetMousePos().y - Anchor.y) / mReach);
            }
            return Hovered != Handle::None;
        }

        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            mHandle = Handle::None;
            return true;
        }

        const Placement Cursor = ToWorld(Composer.GetMousePos(), Origin, Size);
        const Vector2   Delta(
            static_cast<Real32>(Cursor.GetAbsoluteX() - mGrab.GetAbsoluteX()),
            static_cast<Real32>(Cursor.GetAbsoluteY() - mGrab.GetAbsoluteY()));

        Pose Target = mOrigin;

        switch (mMode)
        {
        case Mode::Move:
        {
            const Vector2 Step(
                (mHandle == Handle::AxisY) ? 0.0f : Delta.GetX(),
                (mHandle == Handle::AxisX) ? 0.0f : Delta.GetY());

            if (const ConstPtr<Region> Region = GetRegion(Actor))
            {
                const Real64 TargetX = mStart.GetAbsoluteX() + Step.GetX();
                const Real64 TargetY = mStart.GetAbsoluteY() + Step.GetY();

                Target.SetTranslation(Vector2(
                    static_cast<Real32>(TargetX - Region->GetX() * Tileon::Region::kTilesPerX),
                    static_cast<Real32>(TargetY - Region->GetY() * Tileon::Region::kTilesPerY)));
            }
            break;
        }
        case Mode::Rotate:
        {
            const Real32 Before = Angle::FromCartesian(
                static_cast<Real32>(mGrab.GetAbsoluteX()  - mStart.GetAbsoluteX()),
                static_cast<Real32>(mGrab.GetAbsoluteY()  - mStart.GetAbsoluteY())).GetRadians();
            const Real32 After  = Angle::FromCartesian(
                static_cast<Real32>(Cursor.GetAbsoluteX() - mStart.GetAbsoluteX()),
                static_cast<Real32>(Cursor.GetAbsoluteY() - mStart.GetAbsoluteY())).GetRadians();

            Target.SetRotation(mOrigin.GetRotation() + Angle::FromRadians(After - Before));
            break;
        }
        case Mode::Scale:
        {
            if (mHandle == Handle::Uniform)
            {
                const ImVec2 Offset(Composer.GetMousePos().x - Anchor.x, Composer.GetMousePos().y - Anchor.y);
                const Real32 Reach = Offset.x * mBearing.x + Offset.y * mBearing.y;

                const Vector2 Scale = mOrigin.GetScale() * (Reach / mReach);
                Target.SetScale(Vector2(Max(Scale.GetX(), 0.01f), Max(Scale.GetY(), 0.01f)));
            }
            else
            {
                const Vector2 Step(
                    (mHandle == Handle::AxisY) ? 0.0f : Delta.GetX(),
                    (mHandle == Handle::AxisX) ? 0.0f : Delta.GetY());

                const Vector2 Scale = mOrigin.GetScale() + Step;
                Target.SetScale(Vector2(Max(Scale.GetX(), 0.01f), Max(Scale.GetY(), 0.01f)));
            }
            break;
        }
        }

        Actor.Set(Move(Target));
        Actor.Add<Stale>();

        if (const Scene::Entity Region = Actor.GetParent(); Region.IsValid())
        {
            Region.Add<Tileon::Persist>();
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Gizmo::Handle Gizmo::DrawMove(Ref<UI::Composer> Composer, ImVec2 Anchor) const
    {
        const Ptr<ImDrawList> List   = ImGui::GetWindowDrawList();
        const ImVec2          Cursor = Composer.GetMousePos();

        const ImVec2 EndX(Anchor.x + kAxisLength, Anchor.y);
        const ImVec2 EndY(Anchor.x, Anchor.y - kAxisLength);

        Handle Hovered = Handle::None;

        if (Distance(Cursor, Anchor) <= kPickRadius * 1.6f)
        {
            Hovered = Handle::Plane;
        }
        else if (DistanceToSegment(Cursor, Anchor, EndX) <= kPickRadius)
        {
            Hovered = Handle::AxisX;
        }
        else if (DistanceToSegment(Cursor, Anchor, EndY) <= kPickRadius)
        {
            Hovered = Handle::AxisY;
        }

        const Handle Engaged = (mHandle == Handle::None) ? Hovered : mHandle;

        DrawArrow(List, Anchor, EndX, Engaged == Handle::AxisX ? kColorActive : kColorAxisX);
        DrawArrow(List, Anchor, EndY, Engaged == Handle::AxisY ? kColorActive : kColorAxisY);

        List->AddCircleFilled(Anchor, kPickRadius, Engaged == Handle::Plane ? kColorActive : kColorPlane);
        return Hovered;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Gizmo::Handle Gizmo::DrawRotate(Ref<UI::Composer> Composer, ImVec2 Anchor) const
    {
        const Ptr<ImDrawList> List   = ImGui::GetWindowDrawList();
        const ImVec2          Cursor = Composer.GetMousePos();

        const Real32 Reach   = Distance(Cursor, Anchor);
        const Handle Hovered = (Abs(Reach - kRingRadius) <= kRingBand) ? Handle::Plane : Handle::None;
        const Handle Engaged = (mHandle == Handle::None) ? Hovered : mHandle;
        const UInt32 Color   = (Engaged == Handle::Plane) ? kColorActive : kColorPlane;

        List->AddCircle(Anchor, kRingRadius, Color, 64, Engaged == Handle::Plane ? 3.0f : 2.0f);

        if (Engaged == Handle::Plane && Reach > 0.0001f)
        {
            const ImVec2 Knob(
                Anchor.x + (Cursor.x - Anchor.x) / Reach * kRingRadius,
                Anchor.y + (Cursor.y - Anchor.y) / Reach * kRingRadius);

            List->AddCircleFilled(Knob, 5.0f, Color);
            List->AddLine(Anchor, Knob, Color, 1.0f);
        }
        return Hovered;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Gizmo::Handle Gizmo::DrawScale(Ref<UI::Composer> Composer, ImVec2 Anchor) const
    {
        const Ptr<ImDrawList> List   = ImGui::GetWindowDrawList();
        const ImVec2          Cursor = Composer.GetMousePos();

        const ImVec2 EndX(Anchor.x + kAxisLength, Anchor.y);
        const ImVec2 EndY(Anchor.x, Anchor.y - kAxisLength);

        Handle Hovered = Handle::None;

        // The centre scales both axes by the same factor, so it is the one handle that leaves proportions alone.
        if (Distance(Cursor, Anchor) <= kPickRadius * 1.6f)
        {
            Hovered = Handle::Uniform;
        }
        else if (Distance(Cursor, EndX) <= kPickRadius || DistanceToSegment(Cursor, Anchor, EndX) <= kPickRadius)
        {
            Hovered = Handle::AxisX;
        }
        else if (Distance(Cursor, EndY) <= kPickRadius || DistanceToSegment(Cursor, Anchor, EndY) <= kPickRadius)
        {
            Hovered = Handle::AxisY;
        }

        const Handle Engaged = (mHandle == Handle::None) ? Hovered : mHandle;
        const UInt32 ColorX  = Engaged == Handle::AxisX ? kColorActive : kColorAxisX;
        const UInt32 ColorY  = Engaged == Handle::AxisY ? kColorActive : kColorAxisY;

        constexpr Real32 kBoxExtent = 4.0f;

        List->AddLine(Anchor, EndX, ColorX, 2.0f);
        List->AddRectFilled(ImVec2(EndX.x - kBoxExtent, EndX.y - kBoxExtent), ImVec2(EndX.x + kBoxExtent, EndX.y + kBoxExtent), ColorX);

        List->AddLine(Anchor, EndY, ColorY, 2.0f);
        List->AddRectFilled(ImVec2(EndY.x - kBoxExtent, EndY.y - kBoxExtent), ImVec2(EndY.x + kBoxExtent, EndY.y + kBoxExtent), ColorY);

        // Drawn as a box rather than the move handle's circle, so the uniform grab reads as scaling at a glance.
        const UInt32     Color  = (Engaged == Handle::Uniform) ? kColorActive : kColorPlane;
        constexpr Real32 Extent = kPickRadius * 0.9f;

        List->AddRectFilled(
            ImVec2(Anchor.x - Extent, Anchor.y - Extent), ImVec2(Anchor.x + Extent, Anchor.y + Extent), Color);

        return Hovered;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ConstPtr<Region> Gizmo::GetRegion(Scene::Entity Actor) const
    {
        const Scene::Entity Parent = Actor.GetParent();
        return Parent.IsValid() ? Parent.TryGet<const Tileon::Region>() : nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Placement Gizmo::GetOrigin(Scene::Entity Actor) const
    {
        const ConstPtr<Pose>   Pose   = Actor.TryGet<const Tileon::Pose>();
        const ConstPtr<Region> Region = GetRegion(Actor);

        if (Pose && Region)
        {
            return Placement(Region->GetX(), Region->GetY(), Pose->GetTranslation().GetX(), Pose->GetTranslation().GetY());
        }
        return Placement();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ImVec2 Gizmo::ToScreen(Placement World, ImVec2 Origin, ImVec2 Size) const
    {
        Ref<Director> Director = mContext.GetDirector();

        const Vector2 Pixel  = Director.GetScreenCoordinates(World);
        const Real32  RangeX = Director.GetViewport().GetX() * Director.GetDensity();
        const Real32  RangeY = Director.GetViewport().GetY() * Director.GetDensity();

        return ImVec2(
            Origin.x + (Pixel.GetX() / RangeX) * Size.x,
            Origin.y + (Pixel.GetY() / RangeY) * Size.y);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Placement Gizmo::ToWorld(ImVec2 Screen, ImVec2 Origin, ImVec2 Size) const
    {
        Ref<Director> Director = mContext.GetDirector();

        const Real32 RangeX = Director.GetViewport().GetX() * Director.GetDensity();
        const Real32 RangeY = Director.GetViewport().GetY() * Director.GetDensity();

        const Vector2 Coordinates(((Screen.x - Origin.x) / Size.x) * RangeX, ((Screen.y - Origin.y) / Size.y) * RangeY);
        return Director.GetWorldCoordinates(Coordinates);
    }
}
