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
#include "Tileon.World/Component/Lifecycle.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static ImVec2 AxisTip(ImVec2 Base, ImVec2 Offset, Real32 Length)
    {
        const Real32 Reach = Sqrt(Offset.x * Offset.x + Offset.y * Offset.y);

        if (Reach < 0.0001f)
        {
            return Base;
        }
        return ImVec2(Base.x + Offset.x / Reach * Length, Base.y + Offset.y / Reach * Length);
    }

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

    Bool Gizmo::Draw(ConstRef<Bag<UInt64>> Selection, Scene::Entity Primary, ImVec2 Origin, ImVec2 Size)
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

        // The handles ride at the entity's own height, not on the ground beneath it.
        const Lens   Lens(mContext.GetDirector(), Origin, Size);
        const ImVec2 Anchor = Lens.Project(GetOrigin(Primary), Primary.Get<const Pose>().GetTranslation().GetY());

        Handle Hovered = Handle::None;

        switch (mMode)
        {
        case Mode::Move:
            Hovered = DrawMove(Lens, Anchor);
            break;
        case Mode::Rotate:
            Hovered = DrawRotate(Lens, Anchor);
            break;
        case Mode::Scale:
            Hovered = DrawScale(Lens, Anchor);
            break;
        }

        if (mHandle == Handle::None)
        {
            if (Hovered != Handle::None && Toolkit::Composer::IsMouseClicked(ImGuiMouseButton_Left))
            {
                mHandle    = Hovered;
                mGrab      = Lens.Unproject(Toolkit::Composer::GetMousePos());
                mGrabPixel = Toolkit::Composer::GetMousePos();
                mStart     = GetOrigin(Primary);
                mOrigin    = Primary.Get<const Pose>();
                mReach     = Max(Distance(Toolkit::Composer::GetMousePos(), Anchor), 1.0f);
                mBearing   = ImVec2((Toolkit::Composer::GetMousePos().x - Anchor.x) / mReach, (Toolkit::Composer::GetMousePos().y - Anchor.y) / mReach);

                CaptureSnapshots(Selection, Primary);
            }
            return Hovered != Handle::None;
        }

        if (!Toolkit::Composer::IsMouseDown(ImGuiMouseButton_Left))
        {
            // The runtime only migrates Dynamic entities, so the move is handed to the supervisor here, once, when the drag settles.
            Ref<Supervisor> Supervisor = mContext.GetSupervisor();

            for (ConstRef<Snapshot> Snapshot : mSnapshots)
            {
                const Ptr<Pose>     Pose   = Snapshot.Actor.TryGet<Tileon::Pose>();
                const Scene::Entity Parent = Snapshot.Actor.GetParent();

                if (!Pose)
                {
                    continue;
                }

                // Both regions change: the one that lost the entity and the one that took it in.
                if (const Scene::Entity Target = Supervisor.Migrate(Snapshot.Actor, * Pose); Target.IsValid())
                {
                    Parent.Add<Tileon::Persist>();
                    Target.Add<Tileon::Persist>();
                }
            }

            mHandle = Handle::None;
            mSnapshots.Clear();
            return true;
        }

        const Placement Cursor = Lens.Unproject(Toolkit::Composer::GetMousePos());
        const Vector2   Delta(
            static_cast<Real32>(Cursor.GetAbsoluteX() - mGrab.GetAbsoluteX()),
            static_cast<Real32>(Cursor.GetAbsoluteY() - mGrab.GetAbsoluteY()));

        // Everything turns and scales about the primary's origin, which is where the handles are drawn.
        const Real32 PivotX = static_cast<Real32>(mStart.GetAbsoluteX());
        const Real32 PivotY = static_cast<Real32>(mStart.GetAbsoluteY());

        // A placement only reaches the ground, so the pivot's height comes from the primary's pose instead.
        const Real32 PivotElevation = mOrigin.GetTranslation().GetY();

        // How far the drag has carried along the engaged axis, in world units. Projecting the pixel travel onto
        // the axis's own screen direction is what lets one axis be dragged without disturbing the others.
        const ImVec2 Travel(Toolkit::Composer::GetMousePos().x - mGrabPixel.x, Toolkit::Composer::GetMousePos().y - mGrabPixel.y);
        const ImVec2 Screen = GetAxisOffset(Lens, mHandle);
        const Real32 Square = Screen.x * Screen.x + Screen.y * Screen.y;
        const Real32 Along  = (Square > 0.0001f) ? (Travel.x * Screen.x + Travel.y * Screen.y) / Square : 0.0f;

        // Rotate shares one angle across the group; scale shares one uniform factor. The angle is swept in screen
        // space so every ring turns with the cursor no matter which way its plane happens to face.
        const Real32 Before = Angle::FromCartesian(mGrabPixel.x - Anchor.x, mGrabPixel.y - Anchor.y).GetRadians();
        const Real32 After  = Angle::FromCartesian(
            Toolkit::Composer::GetMousePos().x - Anchor.x, Toolkit::Composer::GetMousePos().y - Anchor.y).GetRadians();

        // Only the rings carry an axis.
        const Vector3    Turn    = GetAxis(mHandle);
        const Angle      Spin    = Angle::FromRadians(After - Before);
        const Quaternion Spinner = Turn.IsAlmostZero() ? Quaternion() : Quaternion::FromAngles(Spin, Turn);

        const ImVec2 ScaleOffset(Toolkit::Composer::GetMousePos().x - Anchor.x, Toolkit::Composer::GetMousePos().y - Anchor.y);
        const Real32 Factor = Max((ScaleOffset.x * mBearing.x + ScaleOffset.y * mBearing.y) / mReach, 0.01f);

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

            // Every handle here works the ground plane, and a placement cannot carry a height, so the elevation
            // is carried over from the snapshot.
            const Real32 Elevation = Snapshot.Origin.GetTranslation().GetY();

            Pose Target = Snapshot.Origin;

            switch (mMode)
            {
            case Mode::Move:
            {
                // The plane handle follows the cursor across the ground; an axis handle carries only along itself.
                const Vector3 Start = Vector3(StartX, Elevation, StartY);
                const Vector3 Step  = (mHandle == Handle::Plane)
                    ? Vector3::FromXZ(Delta)
                    : GetAxis(mHandle) * Along;

                Target.SetTranslation(Start + Step - Vector3::FromXZ(Base));
                break;
            }
            case Mode::Rotate:
            {
                // Orbit the entity's origin about the pivot, and turn it by the same angle in place. Both use the
                // ring's own axis, so a yaw sweeps across the ground while a roll sweeps within the screen.
                const Vector3 Arm     = Vector3(Offset.GetX(), Elevation - PivotElevation, Offset.GetY());
                const Vector3 Rotated = Quaternion::Rotate(Spinner, Arm);
                const Vector3 Pivot   = Vector3(PivotX, PivotElevation, PivotY);

                Target.SetTranslation(Pivot + Rotated - Vector3::FromXZ(Base));
                Target.Rotate(Spinner);
                break;
            }
            case Mode::Scale:
            {
                if (mHandle == Handle::Uniform)
                {
                    // Scale each entity in place and push its origin out from the pivot by the same factor.
                    // A uniform handle takes every axis, elevation included.
                    const Vector3 Scale = Snapshot.Origin.GetScale() * Factor;
                    Target.SetScale(Vector3(
                        Max(Scale.GetX(), 0.01f), Max(Scale.GetY(), 0.01f), Max(Scale.GetZ(), 0.01f)));
                    Target.SetTranslation(Vector3(
                        PivotX + Offset.GetX() * Factor - Base.GetX(),
                        PivotElevation + (Elevation - PivotElevation) * Factor,
                        PivotY + Offset.GetY() * Factor - Base.GetY()));
                }
                else
                {
                    // An axis handle grows only the axis it belongs to. A billboard never offers the depth
                    // handle, so its quad is sized across and up and its unused depth scale is left alone.
                    const Vector3 Scale = Snapshot.Origin.GetScale();
                    const Vector3 Grown = Scale + GetAxis(mHandle) * Along;

                    Target.SetScale(Vector3(
                        Max(Grown.GetX(), 0.01f), Max(Grown.GetY(), 0.01f), Max(Grown.GetZ(), 0.01f)));

                    // An axis scale leaves the origin where it is; keep it pinned in absolute space.
                    Target.SetTranslation(Vector3(StartX - Base.GetX(), Elevation, StartY - Base.GetY()));
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
            mSnapshots.Append(Snapshot(Actor, GetOrigin(Actor), Actor.Get<const Pose>()));
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

    Vector3 Gizmo::GetAxis(Handle Handle)
    {
        switch (Handle)
        {
        case Handle::AxisX:
        case Handle::RingX:
            return Vector3::UnitX();
        case Handle::AxisY:
        case Handle::RingY:
            return Vector3::UnitY();
        case Handle::AxisZ:
        case Handle::RingZ:
            return Vector3::UnitZ();
        default:
            return Vector3();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    ImVec2 Gizmo::GetAxisOffset(ConstRef<Lens> Lens, Handle Handle)
    {
        const Vector3 Axis = GetAxis(Handle);
        return Axis.IsAlmostZero() ? ImVec2(0.0f, 0.0f) : Lens.Direction(Axis);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Gizmo::Handle Gizmo::DrawMove(ConstRef<Lens> Lens, ImVec2 Anchor) const
    {
        const Ptr<ImDrawList> List   = Toolkit::Composer::GetWindowDrawList();
        const ImVec2          Cursor = Toolkit::Composer::GetMousePos();

        // The elevation arrow starts to the side, because under the orthographic camera it points the same way
        // on screen as the depth arrow and would otherwise be impossible to tell apart or grab.
        const ImVec2 LiftBase(Anchor.x + kLiftOffset, Anchor.y);

        const ImVec2 EndX = AxisTip(Anchor,   GetAxisOffset(Lens, Handle::AxisX), kAxisLength);
        const ImVec2 EndZ = AxisTip(Anchor,   GetAxisOffset(Lens, Handle::AxisZ), kAxisLength);
        const ImVec2 EndY = AxisTip(LiftBase, GetAxisOffset(Lens, Handle::AxisY), kAxisLength);

        Handle Hovered = Handle::None;

        if (Distance(Cursor, Anchor) <= kPickRadius * 1.6f)
        {
            Hovered = Handle::Plane;
        }
        else if (DistanceToSegment(Cursor, LiftBase, EndY) <= kPickRadius)
        {
            Hovered = Handle::AxisY;
        }
        else if (DistanceToSegment(Cursor, Anchor, EndX) <= kPickRadius)
        {
            Hovered = Handle::AxisX;
        }
        else if (DistanceToSegment(Cursor, Anchor, EndZ) <= kPickRadius)
        {
            Hovered = Handle::AxisZ;
        }

        const Handle Engaged = (mHandle == Handle::None) ? Hovered : mHandle;

        DrawArrow(List, Anchor,   EndX, Engaged == Handle::AxisX ? Toolkit::Palette::kActive : Toolkit::Palette::kAxisX);
        DrawArrow(List, Anchor,   EndZ, Engaged == Handle::AxisZ ? Toolkit::Palette::kActive : Toolkit::Palette::kAxisZ);
        DrawArrow(List, LiftBase, EndY, Engaged == Handle::AxisY ? Toolkit::Palette::kActive : Toolkit::Palette::kAxisY);

        List->AddCircleFilled(Anchor, kPickRadius, Engaged == Handle::Plane ? Toolkit::Palette::kActive : Toolkit::Palette::kAxisFree);
        return Hovered;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Gizmo::Handle Gizmo::DrawRotate(ConstRef<Lens> Lens, ImVec2 Anchor) const
    {
        const Ptr<ImDrawList> List   = Toolkit::Composer::GetWindowDrawList();
        const ImVec2          Cursor = Toolkit::Composer::GetMousePos();

        // A ring about an axis is drawn as the ellipse that circle projects to, which is spanned by the two
        // world axes it does not turn about. A billboard only gets the screen-plane roll.
        struct Entry final
        {
            Handle Ring;
            Handle First;
            Handle Second;
            UInt32 Color;
        };

        // Ordered so the screen-plane roll is tested first, which is the one a billboard gets on its own.
        constexpr Entry kRings[] =
        {
            { .Ring = Handle::RingZ, .First = Handle::AxisX, .Second = Handle::AxisY, .Color = Toolkit::Palette::kAxisZ },
            { .Ring = Handle::RingY, .First = Handle::AxisX, .Second = Handle::AxisZ, .Color = Toolkit::Palette::kAxisY },
            { .Ring = Handle::RingX, .First = Handle::AxisY, .Second = Handle::AxisZ, .Color = Toolkit::Palette::kAxisX },
        };
        constexpr UInt32 kSegments = 64;

        ImVec2 Points[3][kSegments];
        Handle Hovered = Handle::None;

        for (UInt32 Index = 0; Index < 3; ++Index)
        {
            const ImVec2 First  = GetAxisOffset(Lens, kRings[Index].First);
            const ImVec2 Second = GetAxisOffset(Lens, kRings[Index].Second);

            // Both spanning axes are scaled together so the ellipse keeps the proportions the projection gives
            // it, which is what makes the ring read as lying in its own plane, while still fitting the radius.
            const Real32 Reach = Max(
                Sqrt(First.x  * First.x  + First.y  * First.y),
                Sqrt(Second.x * Second.x + Second.y * Second.y));
            const Real32 Scale = kRingRadius / Max(Reach, 0.0001f);

            for (UInt32 Step = 0; Step < kSegments; ++Step)
            {
                const Real32 Theta = (2.0f * kPI<Real32> * Step) / kSegments;
                const Real32 Cos   = Base::Cosine(Theta);
                const Real32 Sin   = Base::Sine(Theta);

                Points[Index][Step] = ImVec2(
                    Anchor.x + (First.x * Cos + Second.x * Sin) * Scale,
                    Anchor.y + (First.y * Cos + Second.y * Sin) * Scale);
            }

            if (Hovered == Handle::None)
            {
                for (UInt32 Step = 0; Step < kSegments; ++Step)
                {
                    if (DistanceToSegment(Cursor, Points[Index][Step], Points[Index][(Step + 1) % kSegments]) <= kRingBand)
                    {
                        Hovered = kRings[Index].Ring;
                        break;
                    }
                }
            }
        }

        const Handle Engaged = (mHandle == Handle::None) ? Hovered : mHandle;

        for (UInt32 Index = 0; Index < 3; ++Index)
        {
            const Bool Active = (Engaged == kRings[Index].Ring);

            List->AddPolyline(
                Points[Index], kSegments, Active ? Toolkit::Palette::kActive : kRings[Index].Color,
                ImDrawFlags_Closed, Active ? 3.0f : 2.0f);
        }

        if (Engaged != Handle::None)
        {
            const Real32 Reach = Distance(Cursor, Anchor);

            if (Reach > 0.0001f)
            {
                const ImVec2 Knob(
                    Anchor.x + (Cursor.x - Anchor.x) / Reach * kRingRadius,
                    Anchor.y + (Cursor.y - Anchor.y) / Reach * kRingRadius);

                List->AddCircleFilled(Knob, 5.0f, Toolkit::Palette::kActive);
                List->AddLine(Anchor, Knob, Toolkit::Palette::kActive, 1.0f);
            }
        }
        return Hovered;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Gizmo::Handle Gizmo::DrawScale(ConstRef<Lens> Lens, ImVec2 Anchor) const
    {
        const Ptr<ImDrawList> List   = Toolkit::Composer::GetWindowDrawList();
        const ImVec2          Cursor = Toolkit::Composer::GetMousePos();

        const ImVec2 LiftBase(Anchor.x + kLiftOffset, Anchor.y);

        const ImVec2 EndX = AxisTip(Anchor,   GetAxisOffset(Lens, Handle::AxisX), kAxisLength);
        const ImVec2 EndY = AxisTip(LiftBase, GetAxisOffset(Lens, Handle::AxisY), kAxisLength);
        const ImVec2 EndZ = AxisTip(Anchor,   GetAxisOffset(Lens, Handle::AxisZ), kAxisLength);

        Handle Hovered = Handle::None;

        // The center scales every axis by the same factor, so it is the one handle that leaves proportions alone.
        if (Distance(Cursor, Anchor) <= kPickRadius * 1.6f)
        {
            Hovered = Handle::Uniform;
        }
        else if (DistanceToSegment(Cursor, LiftBase, EndY) <= kPickRadius)
        {
            Hovered = Handle::AxisY;
        }
        else if (DistanceToSegment(Cursor, Anchor, EndX) <= kPickRadius)
        {
            Hovered = Handle::AxisX;
        }
        else if (DistanceToSegment(Cursor, Anchor, EndZ) <= kPickRadius)
        {
            Hovered = Handle::AxisZ;
        }

        const Handle Engaged = (mHandle == Handle::None) ? Hovered : mHandle;

        constexpr Real32 kBoxExtent = 4.0f;

        const auto DrawStub = [&](ImVec2 Base, ImVec2 End, Handle Which, UInt32 Tone)
        {
            const UInt32 Color = (Engaged == Which) ? Toolkit::Palette::kActive : Tone;

            List->AddLine(Base, End, Color, 2.0f);
            List->AddRectFilled(
                ImVec2(End.x - kBoxExtent, End.y - kBoxExtent), ImVec2(End.x + kBoxExtent, End.y + kBoxExtent), Color);
        };

        DrawStub(Anchor,   EndX, Handle::AxisX, Toolkit::Palette::kAxisX);
        DrawStub(LiftBase, EndY, Handle::AxisY, Toolkit::Palette::kAxisY);
        DrawStub(Anchor,   EndZ, Handle::AxisZ, Toolkit::Palette::kAxisZ);

        // Drawn as a box rather than the move handle's circle, so the uniform grab reads as scaling at a glance.
        const UInt32     Color  = (Engaged == Handle::Uniform) ? Toolkit::Palette::kActive : Toolkit::Palette::kAxisFree;
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
            // A placement addresses the ground, so the pose has to be projected onto it. Reading the translation's
            // Y here would hand it the elevation instead of the ground depth.
            const Vector2 Ground = Pose->GetTranslation().GetXZ();
            return Placement(Region->GetX(), Region->GetY(), Ground.GetX(), Ground.GetY());
        }
        return Placement();
    }
}