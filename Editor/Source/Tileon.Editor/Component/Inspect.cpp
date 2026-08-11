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

#include "Inspect.hpp"
#include "Tileon.Editor/Context.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectVector(Text Label, Ref<Vector2> Value, Real32 Speed = 1.0f, Real32 Min = 0.0f, Real32 Max = 0.0f)
    {
        Real32 X = Value.GetX();
        Real32 Y = Value.GetY();

        Toolkit::Composer::FieldInline(Label);
        Toolkit::Composer::PushID(Label);

        const Bool Dirty = Toolkit::Composer::InputFloatPair("##value", X, Y, "%.3f", Speed, Min, Max);

        Toolkit::Composer::PopID();

        if (Dirty)
        {
            Value.Set(X, Y);
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectVector(Text Label, Ref<Vector3> Value, Real32 Speed = 1.0f, Real32 Min = 0.0f, Real32 Max = 0.0f)
    {
        Real32 X = Value.GetX();
        Real32 Y = Value.GetY();
        Real32 Z = Value.GetZ();

        Toolkit::Composer::FieldInline(Label);
        Toolkit::Composer::PushID(Label);

        const Bool Dirty = Toolkit::Composer::InputFloatTriple("##value", X, Y, Z, "%.3f", Speed, Min, Max);

        Toolkit::Composer::PopID();

        if (Dirty)
        {
            Value.Set(X, Y, Z);
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectEuler(Text Label, Ref<Vector3> Value, Text Format = "%.2f°")
    {
        Real32 X = Angle(Value.GetX()).GetDegrees();
        Real32 Y = Angle(Value.GetY()).GetDegrees();
        Real32 Z = Angle(Value.GetZ()).GetDegrees();

        Toolkit::Composer::FieldInline(Label);
        Toolkit::Composer::PushID(Label);

        const Bool Dirty = Toolkit::Composer::InputFloatTriple("##value", X, Y, Z, Format, 0.5f);

        Toolkit::Composer::PopID();

        if (Dirty)
        {
            Value.Set(
                Angle::FromDegrees(X).GetRadians(),
                Angle::FromDegrees(Y).GetRadians(),
                Angle::FromDegrees(Z).GetRadians());
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectScalar(Text Label, Ref<Real32> Value, Real32 Speed = 0.1f)
    {
        Toolkit::Composer::FieldInline(Label);
        Toolkit::Composer::PushID(Label);

        const Bool Dirty = Toolkit::Composer::DragFloat("##value", Value, Speed);

        Toolkit::Composer::PopID();

        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectAngle(Text Label, Ref<Angle> Value, Real32 Min = 0.0f, Real32 Max = 360.0f)
    {
        Real32 Radians = Value.GetRadians();

        Toolkit::Composer::FieldInline(Label);
        Toolkit::Composer::PushID(Label);

        const Bool Dirty = Toolkit::Composer::SliderAngle("##value", Radians, Min, Max);

        Toolkit::Composer::PopID();

        if (Dirty)
        {
            Value = Angle(Radians);
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectTint(Text Label, Ref<IntColor8> Value)
    {
        Toolkit::Composer::FieldInline(Label);
        Toolkit::Composer::PushID(Label);

        const Bool Dirty = Toolkit::Composer::InputTintSmall("##value", Value);

        Toolkit::Composer::PopID();

        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectRect(Text Label, Ref<Rect> Value, Real32 ScaleX = 1.0f, Real32 ScaleY = 1.0f, Real32 Speed = 1.0f)
    {
        Real32 MinimumX = Value.GetMinimumX() * ScaleX;
        Real32 MinimumY = Value.GetMinimumY() * ScaleY;
        Real32 MaximumX = Value.GetMaximumX() * ScaleX;
        Real32 MaximumY = Value.GetMaximumY() * ScaleY;

        Toolkit::Composer::PushID(Label);

        Toolkit::Composer::Field(String<64>::Print<"{0} (Min)">(Label));
        Bool Dirty = Toolkit::Composer::InputFloatPair("##minimum", MinimumX, MinimumY, "%.2f", Speed);

        Toolkit::Composer::Field(String<64>::Print<"{0} (Max)">(Label));
        Dirty |= Toolkit::Composer::InputFloatPair("##maximum", MaximumX, MaximumY, "%.2f", Speed);

        Toolkit::Composer::PopID();

        if (Dirty)
        {
            Value.Set(
                ScaleX != 0.0f ? MinimumX / ScaleX : MinimumX,
                ScaleY != 0.0f ? MinimumY / ScaleY : MinimumY,
                ScaleX != 0.0f ? MaximumX / ScaleX : MaximumX,
                ScaleY != 0.0f ? MaximumY / ScaleY : MaximumY);
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectSunDirection(Text Label, Ref<Vector3> Value)
    {
        constexpr Real32 kSize = 104.0f;

        Toolkit::Composer::FieldInline(Label);
        Toolkit::Composer::PushID(Label);

        constexpr Real32 Radius = kSize * 0.5f - 6.0f;
        const     ImVec2 Origin = Toolkit::Composer::GetCursorScreenPos();
        const     ImVec2 Center(Origin.x + kSize * 0.5f, Origin.y + kSize * 0.5f);

        Toolkit::Composer::InvisibleButton("##sun", ImVec2(kSize, kSize));

        Bool Dirty = false;

        if (Toolkit::Composer::IsItemActive())
        {
            const ImVec2 Mouse  = Toolkit::Composer::GetMousePos();
            const Real32 PlaneX = (Mouse.x - Center.x) / Radius;
            const Real32 PlaneZ = (Mouse.y - Center.y) / Radius;

            const Real32 Length = Sqrt(PlaneX * PlaneX + PlaneZ * PlaneZ);
            const Real32 Scale  = (Length > 1.0f) ? (1.0f / Length) : 1.0f;

            const Real32 GroundX = PlaneX * Scale;
            const Real32 GroundZ = PlaneZ * Scale;

            Value = Vector3(GroundX, Sqrt(Max(1.0f - (GroundX * GroundX + GroundZ * GroundZ), 0.0f)), GroundZ);
            Dirty = true;
        }

        const Vector3 Sun = Vector3::Normalize(Value.IsAlmostZero() ? Vector3(0.0f, 1.0f, 0.0f) : Value);
        const ImVec2  Handle(Center.x + Sun.GetX() * Radius, Center.y + Sun.GetZ() * Radius);

        const Ptr<ImDrawList> List = Toolkit::Composer::GetWindowDrawList();

        List->AddCircleFilled(Center, Radius, IM_COL32(22, 26, 34, 255));
        List->AddCircle(Center, Radius, IM_COL32(92, 102, 122, 255));
        List->AddLine(ImVec2(Center.x - Radius, Center.y), ImVec2(Center.x + Radius, Center.y), IM_COL32(64, 72, 88, 255));
        List->AddLine(ImVec2(Center.x, Center.y - Radius), ImVec2(Center.x, Center.y + Radius), IM_COL32(64, 72, 88, 255));

        const Bool Above = (Sun.GetY() >= 0.0f);

        List->AddLine(Center, Handle, IM_COL32(255, 210, 120, 150));
        List->AddCircleFilled(Handle, 5.0f, Above ? IM_COL32(255, 210, 120, 255) : IM_COL32(40, 44, 54, 255));
        List->AddCircle(Handle, 5.0f, IM_COL32(255, 210, 120, 255));

        // The dial only steers the ground plane, so the axes sit beside it for the elevation and exact figures.
        Real32 X = Value.GetX();
        Real32 Y = Value.GetY();
        Real32 Z = Value.GetZ();

        const Real32 Stack = Toolkit::Composer::GetFrameHeight() * 3.0f + Toolkit::Composer::GetStyle().ItemSpacing.y * 2.0f;

        Toolkit::Composer::SameLine();
        Toolkit::Composer::SetCursorPosY(Toolkit::Composer::GetCursorPosY() + (kSize - Stack) * 0.5f);

        Toolkit::Composer::BeginGroup();
        {
            const Real32 Width = Toolkit::Composer::GetContentRegionAvail().x;

            Dirty |= Toolkit::Composer::InputFloatAxis("##x", "X", Toolkit::Composer::kAxisTintX, X, Width);
            Dirty |= Toolkit::Composer::InputFloatAxis("##y", "Y", Toolkit::Composer::kAxisTintY, Y, Width);
            Dirty |= Toolkit::Composer::InputFloatAxis("##z", "Z", Toolkit::Composer::kAxisTintZ, Z, Width);
        }
        Toolkit::Composer::EndGroup();

        if (Dirty)
        {
            Value.Set(X, Y, Z);
        }

        Toolkit::Composer::PopID();
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectPivot(Text Label, Ref<Pivot2D> Value)
    {
        struct Preset final
        {
            Text    Name;
            Pivot2D Pivot;
        };

        static constexpr Array kPresets = {
            Preset { .Name = "Left Top",      .Pivot = Pivot2D::LeftTop()      },
            Preset { .Name = "Left Middle",   .Pivot = Pivot2D::LeftMiddle()   },
            Preset { .Name = "Left Bottom",   .Pivot = Pivot2D::LeftBottom()   },
            Preset { .Name = "Center Top",    .Pivot = Pivot2D::CenterTop()    },
            Preset { .Name = "Center Middle", .Pivot = Pivot2D::CenterMiddle() },
            Preset { .Name = "Center Bottom", .Pivot = Pivot2D::CenterBottom() },
            Preset { .Name = "Right Top",     .Pivot = Pivot2D::RightTop()     },
            Preset { .Name = "Right Middle",  .Pivot = Pivot2D::RightMiddle()  },
            Preset { .Name = "Right Bottom",  .Pivot = Pivot2D::RightBottom()  },
        };

        static constexpr Text kCustom = "Custom";

        const auto Matches = [&Value](ConstRef<Preset> Option)
        {
            return IsAlmostEqual(Option.Pivot.GetX(), Value.GetX())
                && IsAlmostEqual(Option.Pivot.GetY(), Value.GetY());
        };

        Toolkit::Composer::FieldInline(Label);
        Toolkit::Composer::PushID(Label);

        ConstPtr<Preset> Match = nullptr;

        for (ConstRef<Preset> Option : kPresets)
        {
            if (Matches(Option))
            {
                Match = AddressOf(Option);
                break;
            }
        }

        Bool Custom = !Match || Toolkit::Composer::GetState("custom");
        Bool Dirty  = false;

        if (Toolkit::Composer::BeginCombo("##value", Custom ? kCustom : Match->Name))
        {
            for (ConstRef<Preset> Option : kPresets)
            {
                if (Toolkit::Composer::Selectable(Option.Name, !Custom && Matches(Option)))
                {
                    Value  = Option.Pivot;
                    Custom = false;
                    Dirty  = true;
                }
            }

            if (Toolkit::Composer::Selectable(kCustom, Custom))
            {
                Custom = true;
            }
            Toolkit::Composer::EndCombo();
        }

        Toolkit::Composer::SetState("custom", Custom);

        if (Custom)
        {
            Real32 X = Value.GetX();
            Real32 Y = Value.GetY();

            if (Toolkit::Composer::InputFloatPair("##custom", X, Y, "%.3f", 0.01f))
            {
                Value = Pivot2D(X, Y);
                Dirty = true;
            }
        }

        Toolkit::Composer::PopID();

        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Callback>
    static void InspectAsset(
        Ref<Workspace>         Workspace,
        Scene::Entity          Actor,
        Text                   Label,
        Text                   Filter,
        ConstRef<Content::Uri> Value,
        AnyRef<Callback>       Action)
    {
        Ref<Toolkit::Selector> Selector = Workspace.Selector;

        const UInt64 Key = Toolkit::Composer::IsDisabled() ? 0 : HashCombine(Label, Actor.GetID());

        if (Str Selection; Selector.Consume(Key, Selection))
        {
            Action(Content::Uri(Move(Selection)));
        }

        Toolkit::Composer::FieldInline(Label);
        Toolkit::Composer::PushID(Label);
        Toolkit::Composer::InputTextWithButton("##value", Value.GetPath(),
            [&](Text Path)
            {
                Action(Content::Uri(Str::Print<"Resources://{0}">(Path)));
            },
            ICON_FA_ELLIPSIS,
            [&]
            {
                Selector.Open(Key, Filter);
            },
            ImGuiInputTextFlags_EnterReturnsTrue);
        Toolkit::Composer::PopID();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Anchor> Component)
    {
        const Real32 Density = Workspace.Context.GetDirector().GetDensity();

        Vector3 Value = Component.GetValue() * Density;

        if (InspectVector("Value", Value))
        {
            Component.SetValue(Value / Density);
            return true;
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Extent> Component)
    {
        const Real32 Density = Workspace.Context.GetDirector().GetDensity();

        Bool Dirty = false;

        if (Vector3 Offset = Component.GetOffset() * Density; InspectVector("Offset", Offset))
        {
            Component.SetOffset(Offset / Density);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (Vector3 Size = Component.GetSize() * Density; InspectVector("Size", Size))
        {
            Component.SetSize(Size / Density);
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Pose> Component)
    {
        const Real32 Density = Workspace.Context.GetDirector().GetDensity();

        Bool Dirty = false;

        Vector3 Base = Vector3::Zero();

        if (const ConstPtr<Tileon::Transform> Transform = Actor.TryGet<const Tileon::Transform>())
        {
            Base = Vector3(Transform->GetOrigin());
        }

        if (Vector3 Translation = (Component.GetTranslation() + Base) * Density; InspectVector("Translation", Translation))
        {
            Component.SetTranslation(Translation / Density - Base);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (Vector3 Scale = Component.GetScale(); InspectVector("Scale", Scale, 0.01f, 0.0f, FLT_MAX))
        {
            Component.SetScale(Scale);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (Vector3 Rotation = Component.GetEulerAngles(); InspectEuler("Rotation", Rotation))
        {
            Component.SetEulerAngles(Rotation);
            Dirty = true;
        }

        if (Dirty)
        {
            Actor.Add<Stale>();
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Velocity> Component)
    {
        Bool Dirty = false;

        if (Vector3 Linear = Component.GetLinear(); InspectVector("Linear", Linear))
        {
            Component.SetLinear(Linear);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (Vector3 Angular = Component.GetAngular(); InspectEuler("Angular", Angular, "%.2f°/s"))
        {
            Component.SetAngular(Angular);
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Glowlight> Component)
    {
        Bool Dirty = false;

        if (Real32 Radius = Component.GetRadius(); InspectScalar("Radius", Radius))
        {
            Component.SetRadius(Radius);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (Real32 Intensity = Component.GetIntensity(); InspectScalar("Intensity", Intensity))
        {
            Component.SetIntensity(Intensity);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (Real32 Falloff = Component.GetFalloff(); InspectScalar("Falloff", Falloff))
        {
            Component.SetFalloff(Falloff);
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Spotlight> Component)
    {
        Bool  Dirty = false;
        Angle Inner = Component.GetInnerAngle();
        Angle Outer = Component.GetOuterAngle();

        if (InspectAngle("Inner Angle", Inner))
        {
            Component.SetAngles(Inner, Outer);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (InspectAngle("Outer Angle", Outer))
        {
            Component.SetAngles(Inner, Outer);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (Real32 Range = Component.GetRange(); InspectScalar("Range", Range))
        {
            Component.SetRange(Range);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (Real32 Intensity = Component.GetIntensity(); InspectScalar("Intensity", Intensity))
        {
            Component.SetIntensity(Intensity);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (Real32 Falloff = Component.GetFalloff(); InspectScalar("Falloff", Falloff))
        {
            Component.SetFalloff(Falloff);
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Skylight> Component)
    {
        Bool Dirty = false;

        if (Vector3 Direction = Component.GetSunDirection(); InspectSunDirection("Sun", Direction))
        {
            Component.SetSunDirection(Direction);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (IntColor8 Tint = Component.GetSunTint(); InspectTint("Sun Tint", Tint))
        {
            Component.SetSunTint(Tint);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (IntColor8 Tint = Component.GetSkyTint(); InspectTint("Sky Tint", Tint))
        {
            Component.SetSkyTint(Tint);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (IntColor8 Tint = Component.GetGroundTint(); InspectTint("Ground Tint", Tint))
        {
            Component.SetGroundTint(Tint);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (Real32 Brightness = Component.GetBrightness(); InspectScalar("Brightness", Brightness))
        {
            Component.SetBrightness(Brightness);
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Sprite> Component)
    {
        Bool Dirty = false;

        InspectAsset(Workspace, Actor, "Path", ".mtl", Component.GetPath(),
            [&](AnyRef<Content::Uri> Path)
            {
                Component.SetPath(Move(Path));
                Dirty = true;
            });

        Toolkit::Composer::Spacing();

        Real32 ScaleX = 1.0f;
        Real32 ScaleY = 1.0f;

        ConstRetainer<Graphic::Material> Material = Workspace.Context.GetContent().Load<Graphic::Material>(Component.GetPath());

        if (Material && Material->HasCompleted())
        {
            ConstRetainer<Graphic::Image> Albedo = Material->GetImage("Albedo"_Hash);

            if (Albedo && Albedo->GetWidth() > 0 && Albedo->GetHeight() > 0)
            {
                ScaleX = Albedo->GetWidth();
                ScaleY = Albedo->GetHeight();
            }
        }

        if (Rect Source = Component.GetSource(); InspectRect("Source", Source, ScaleX, ScaleY))
        {
            Component.SetSource(Source);
            Dirty = true;
        }

        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Animation> Component)
    {
        Bool Dirty = false;

        Toolkit::Composer::TextDisabled("{0} frame(s), {1:.2f}s total", Component.GetCount(), Component.GetDuration());
        Toolkit::Composer::Spacing();

        if (Toolkit::Composer::DisabledButton(ICON_FA_PLUS "  Add Frame", Component.IsFull(), -1.0f))
        {
            Component.Insert(Rect(0.0f, 0.0f, 1.0f, 1.0f), 0.1f);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (Component.IsEmpty())
        {
            constexpr Text kHint = "No Frames";

            Toolkit::Composer::SetCursorPosX((Toolkit::Composer::GetContentRegionAvail().x - Toolkit::Composer::CalcTextSize(kHint).x) * 0.5f);
            Toolkit::Composer::TextDisabled(kHint);
            return Dirty;
        }

        constexpr ImGuiTableFlags kTableFlags =
            ImGuiTableFlags_BordersOuter  |
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_RowBg         |
            ImGuiTableFlags_SizingStretchSame;

        if (Toolkit::Composer::BeginTable("##frames", 7, kTableFlags))
        {
            Toolkit::Composer::TableSetupColumn("#",    ImGuiTableColumnFlags_WidthFixed,   18.0f);
            Toolkit::Composer::TableSetupColumn("X",    ImGuiTableColumnFlags_WidthStretch);
            Toolkit::Composer::TableSetupColumn("Y",    ImGuiTableColumnFlags_WidthStretch);
            Toolkit::Composer::TableSetupColumn("W",    ImGuiTableColumnFlags_WidthStretch);
            Toolkit::Composer::TableSetupColumn("H",    ImGuiTableColumnFlags_WidthStretch);
            Toolkit::Composer::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthStretch);
            Toolkit::Composer::TableSetupColumn("",     ImGuiTableColumnFlags_WidthFixed,   16.0f);
            Toolkit::Composer::TableHeadersRow();

            UInt8 Discard = Animation::kMaxFrames;

            for (UInt8 Keyframe = 0; Keyframe < Component.GetCount(); ++Keyframe)
            {
                const Rect Data     = Component.GetFrameData(Keyframe);
                Real32     X        = Data.GetMinimumX();
                Real32     Y        = Data.GetMinimumY();
                Real32     Width    = Data.GetWidth();
                Real32     Height   = Data.GetHeight();
                Real32     Duration = Component.GetFrameDuration(Keyframe);

                Toolkit::Composer::PushID(Keyframe);
                Toolkit::Composer::TableNextRow();

                Toolkit::Composer::TableSetColumnIndex(0);
                Toolkit::Composer::Label("{0}", Keyframe + 1);

                const auto OnCell = [&](SInt32 Column, Text ID, Ref<Real32> Value, Text Format)
                {
                    Toolkit::Composer::TableSetColumnIndex(Column);
                    Toolkit::Composer::SetNextItemWidth(-1.0f);
                    return Toolkit::Composer::InputFloat(ID, Value, 0.0f, 0.0f, Format);
                };

                Bool Enclosure = OnCell(1, "##x", X, "%.3f");
                Enclosure |= OnCell(2, "##y", Y, "%.3f");
                Enclosure |= OnCell(3, "##w", Width, "%.3f");
                Enclosure |= OnCell(4, "##h", Height, "%.3f");

                if (Enclosure)
                {
                    Component.SetFrameData(Keyframe, Rect(X, Y, X + Width, Y + Height));
                    Dirty = true;
                }

                if (OnCell(5, "##t", Duration, "%.2f"))
                {
                    Component.SetFrameDuration(Keyframe, Max(Duration, 0.01f));
                    Dirty = true;
                }

                Toolkit::Composer::TableSetColumnIndex(6);

                if (Toolkit::Composer::SmallButton(ICON_FA_XMARK))
                {
                    Discard = Keyframe;
                }

                Toolkit::Composer::PopID();
            }

            Toolkit::Composer::EndTable();

            if (Discard < Component.GetCount())
            {
                Component.Remove(Discard);
                Dirty = true;
            }
        }
        return Dirty;
    }


    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Lettering> Component)
    {
        const Real32 Density = Workspace.Context.GetDirector().GetDensity();

        Bool Dirty = false;

        ConstRetainer<::Render::Font> Font = Component.GetFont();

        InspectAsset(Workspace, Actor, "Font", ".fnt", Font ? Font->GetKey() : Content::Uri(),
            [&](AnyRef<Content::Uri> Path)
            {
                Component.SetFont(Move(Path));
                Dirty = true;
            });

        Toolkit::Composer::Spacing();

        if (Real32 Size = Component.GetSize() * Density; InspectScalar("Size", Size))
        {
            Component.SetSize(Size / Density);
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Label> Component)
    {
        const Real32 Density = Workspace.Context.GetDirector().GetDensity();

        Bool Dirty = false;

        Toolkit::Composer::Field("Content");
        Toolkit::Composer::PushID("Content");
        Toolkit::Composer::SetNextItemWidth(-1.0f);
        Toolkit::Composer::InputText("##value", Component.GetContent(), [&](Text Value)
        {
            Component.SetContent(Value);
            Dirty = true;
        });
        Toolkit::Composer::PopID();

        Toolkit::Composer::Spacing();

        if (Vector2 Spacing = Component.GetSpacing() * Density; InspectVector("Spacing", Spacing))
        {
            Component.SetSpacing(Spacing / Density);
            Dirty = true;
        }

        Toolkit::Composer::Spacing();

        if (Pivot2D Pivot = Component.GetPivot(); InspectPivot("Pivot", Pivot))
        {
            Component.SetPivot(Pivot);
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<Workspace> Workspace, Scene::Entity Actor, Ref<IntColor8> Component)
    {
        return InspectTint("Color", Component);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Decoration> Component)
    {
        ConstRef<Render::FontEffect> Effect = Component.GetEffect();

        Color  OutsetColor     = Effect.GetOutsetColor();
        Real32 OutsetOffset    = Effect.GetOutsetOffset();
        Real32 OutsetWidth     = Effect.GetOutsetWidthRelative();
        Real32 OutsetBias      = Effect.GetOutsetWidthAbsolute();
        Real32 OutsetBlur      = Effect.GetOutsetBlur();
        Real32 InsetRoundness  = Effect.GetInsetRoundness();
        Real32 InsetThreshold  = Effect.GetInsetThreshold();

        Bool Dirty = false;

        Toolkit::Composer::FieldInline("Outset Color");
        Toolkit::Composer::PushID("Outset Color");
        Dirty |= Toolkit::Composer::InputTintSmall("##value", OutsetColor);
        Toolkit::Composer::PopID();

        Dirty |= InspectScalar("Outset Offset",    OutsetOffset);
        Dirty |= InspectScalar("Outset Width",     OutsetWidth);
        Dirty |= InspectScalar("Outset Bias",      OutsetBias);
        Dirty |= InspectScalar("Outset Blur",      OutsetBlur);
        Dirty |= InspectScalar("Inset Roundness",  InsetRoundness);
        Dirty |= InspectScalar("Inset Threshold",  InsetThreshold);

        if (Dirty)
        {
            Component.SetEffect(Render::FontEffect(
                OutsetColor, OutsetOffset, OutsetWidth, OutsetBias, OutsetBlur, InsetRoundness, InsetThreshold));
        }
        return Dirty;
    }
}