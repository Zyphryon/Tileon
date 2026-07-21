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
#include <Zyphryon.Math/Matrix3x2.hpp>

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

    Bool Gizmo::Draw(Ref<UI::Composer> Composer, ConstRef<Bag<UInt64>> Selection, Scene::Entity Primary, ImVec2 Origin, ImVec2 Size)
    {
        // The handles anchor on the primary: whether the selection is the instance root or one of its parts,
        // transform the root, whose Pose cascades down to every part through World::ComputeWorldspace.
        Primary = Primary.IsValid() ? Scene::Entity::ResolveRecursively(Primary, Scene::Hierarchy::Fixed) : Primary;

        if (!Primary.IsValid() || !Primary.Has<Pose>())
        {
            mHandle = Handle::None;
            mSnapshots.Clear();
            return false;
        }

        const Lens   Lens   = { mContext.GetDirector(), Origin, Size };
        const ImVec2 Anchor = Lens.Project(GetOrigin(Primary));

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
                mGrab    = Lens.Unproject(Composer.GetMousePos());
                mStart   = GetOrigin(Primary);
                mOrigin  = Primary.Get<const Pose>();
                mReach   = Max(Distance(Composer.GetMousePos(), Anchor), 1.0f);
                mBearing = ImVec2(
                    (Composer.GetMousePos().x - Anchor.x) / mReach, (Composer.GetMousePos().y - Anchor.y) / mReach);

                CaptureSnapshots(Selection, Primary);
            }
            return Hovered != Handle::None;
        }

        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            mHandle = Handle::None;
            mSnapshots.Clear();
            return true;
        }

        const Placement Cursor = Lens.Unproject(Composer.GetMousePos());
        const Vector2   Delta(
            static_cast<Real32>(Cursor.GetAbsoluteX() - mGrab.GetAbsoluteX()),
            static_cast<Real32>(Cursor.GetAbsoluteY() - mGrab.GetAbsoluteY()));

        // Everything turns and scales about the primary's origin, which is where the handles are drawn.
        const Real32 PivotX = static_cast<Real32>(mStart.GetAbsoluteX());
        const Real32 PivotY = static_cast<Real32>(mStart.GetAbsoluteY());

        // Rotate shares one angle across the group; scale shares one uniform factor.
        const Real32 Before = Angle::FromCartesian(
            static_cast<Real32>(mGrab.GetAbsoluteX()  - mStart.GetAbsoluteX()),
            static_cast<Real32>(mGrab.GetAbsoluteY()  - mStart.GetAbsoluteY())).GetRadians();
        const Real32 After  = Angle::FromCartesian(
            static_cast<Real32>(Cursor.GetAbsoluteX() - mStart.GetAbsoluteX()),
            static_cast<Real32>(Cursor.GetAbsoluteY() - mStart.GetAbsoluteY())).GetRadians();

        // Reuse the engine's rotation matrix so an orbit always turns the same way the sprite itself does.
        const Angle     Spin    = Angle::FromRadians(After - Before);
        const Matrix3x2 Spinner = Matrix3x2::FromRotation(Spin);

        const ImVec2 ScaleOffset(Composer.GetMousePos().x - Anchor.x, Composer.GetMousePos().y - Anchor.y);
        const Real32 Factor = (ScaleOffset.x * mBearing.x + ScaleOffset.y * mBearing.y) / mReach;

        for (ConstRef<Snapshot> Snapshot : mSnapshots)
        {
            Vector2 Base;

            if (const ConstPtr<Region> Region = GetRegion(Snapshot.Actor))
            {
                Base = Vector2(
                    static_cast<Real32>(Region->GetX() * Tileon::Region::kTilesPerX),
                    static_cast<Real32>(Region->GetY() * Tileon::Region::kTilesPerY));
            }

            const Real32   StartX = static_cast<Real32>(Snapshot.Start.GetAbsoluteX());
            const Real32   StartY = static_cast<Real32>(Snapshot.Start.GetAbsoluteY());
            const Vector2  Offset(StartX - PivotX, StartY - PivotY);

            Pose Target = Snapshot.Origin;

            switch (mMode)
            {
            case Mode::Move:
            {
                const Vector2 Step(
                    (mHandle == Handle::AxisY) ? 0.0f : Delta.GetX(),
                    (mHandle == Handle::AxisX) ? 0.0f : Delta.GetY());

                Target.SetTranslation(Vector2(StartX + Step.GetX() - Base.GetX(), StartY + Step.GetY() - Base.GetY()));
                break;
            }
            case Mode::Rotate:
            {
                // Orbit the entity's origin about the pivot, and turn it by the same angle in place.
                const Vector2 Rotated = Matrix3x2::Project(Spinner, Offset);

                Target.SetTranslation(Vector2(PivotX + Rotated.GetX() - Base.GetX(), PivotY + Rotated.GetY() - Base.GetY()));
                Target.SetRotation(Snapshot.Origin.GetRotation() + Spin);
                break;
            }
            case Mode::Scale:
            {
                if (mHandle == Handle::Uniform)
                {
                    // Scale each entity in place and push its origin out from the pivot by the same factor.
                    const Vector2 Scale = Snapshot.Origin.GetScale() * Factor;
                    Target.SetScale(Vector2(Max(Scale.GetX(), 0.01f), Max(Scale.GetY(), 0.01f)));
                    Target.SetTranslation(Vector2(
                        PivotX + Offset.GetX() * Factor - Base.GetX(),
                        PivotY + Offset.GetY() * Factor - Base.GetY()));
                }
                else
                {
                    const Vector2 Step(
                        (mHandle == Handle::AxisY) ? 0.0f : Delta.GetX(),
                        (mHandle == Handle::AxisX) ? 0.0f : Delta.GetY());

                    const Vector2 Scale = Snapshot.Origin.GetScale() + Step;
                    Target.SetScale(Vector2(Max(Scale.GetX(), 0.01f), Max(Scale.GetY(), 0.01f)));

                    // An axis scale leaves the origin where it is; keep it pinned in absolute space.
                    Target.SetTranslation(Vector2(StartX - Base.GetX(), StartY - Base.GetY()));
                }
                break;
            }
            }

            Snapshot.Actor.Set(Move(Target));
            Snapshot.Actor.Add<Stale>();

            if (const Scene::Entity Region = Snapshot.Actor.GetParent(); Region.IsValid())
            {
                Region.Add<Tileon::Persist>();
            }
        }
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Gizmo::CaptureSnapshots(ConstRef<Bag<UInt64>> Selection, Scene::Entity Primary)
    {
        mSnapshots.Clear();

        Ref<Scene::Service> Scene = mContext.GetScene();

        const auto Capture = [&](Scene::Entity Actor)
        {
            if (!Actor.IsValid())
            {
                return;
            }
            Actor = Scene::Entity::ResolveRecursively(Actor, Scene::Hierarchy::Fixed);

            if (!Actor.Has<Pose>())
            {
                return;
            }

            // Resolving parts to their root can collapse several ids onto one entity, so guard against duplicates.
            for (ConstRef<Snapshot> Existing : mSnapshots)
            {
                if (Existing.Actor.GetID() == Actor.GetID())
                {
                    return;
                }
            }
            mSnapshots.Append(Snapshot { Actor, GetOrigin(Actor), Actor.Get<const Pose>() });
        };

        if (Selection.IsEmpty())
        {
            Capture(Primary);
        }
        else
        {
            for (const UInt64 ID : Selection)
            {
                Capture(Scene.GetEntity(ID));
            }
        }
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
}
