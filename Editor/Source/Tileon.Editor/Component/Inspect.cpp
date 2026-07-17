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

    static Bool InspectVector(Ref<UI::Composer> Composer, Text Label, Ref<Vector2> Value)
    {
        Real32 X = Value.GetX();
        Real32 Y = Value.GetY();

        Composer.Field(Label);
        Composer.PushID(Label);

        const Bool Dirty = Composer.InputFloatPair("##value", X, Y, "%.5f");

        Composer.PopID();

        if (Dirty)
        {
            Value.Set(X, Y);
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectScalar(Ref<UI::Composer> Composer, Text Label, Ref<Real32> Value)
    {
        Composer.Field(Label);
        Composer.PushID(Label);
        Composer.SetNextItemWidth(-1.0f);

        const Bool Dirty = Composer.InputFloat("##value", Value);

        Composer.PopID();

        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectAngle(Ref<UI::Composer> Composer, Text Label, Ref<Angle> Value, Real32 Min = 0.0f, Real32 Max = 360.0f)
    {
        Real32 Radians = Value.GetRadians();

        Composer.Field(Label);
        Composer.PushID(Label);
        Composer.SetNextItemWidth(-1.0f);

        const Bool Dirty = Composer.SliderAngle("##value", Radians, Min, Max);

        Composer.PopID();

        if (Dirty)
        {
            Value = Angle(Radians);
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectAngularRate(Ref<UI::Composer> Composer, Text Label, Ref<Angle> Value)
    {
        Real32 Degrees = Value.GetDegrees();

        Composer.Field(Label);
        Composer.PushID(Label);
        Composer.SetNextItemWidth(-1.0f);

        const Bool Dirty = Composer.InputFloat("##value", Degrees, 0.0f, 0.0f, "%.2f°/s");

        Composer.PopID();

        if (Dirty)
        {
            Value = Angle::FromDegrees(Degrees);
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectTint(Ref<UI::Composer> Composer, Text Label, Ref<IntColor8> Value)
    {
        Composer.Field(Label);
        Composer.PushID(Label);
        Composer.SetNextItemWidth(-1.0f);

        const Bool Dirty = Composer.InputTintSmall("##value", Value);

        Composer.PopID();

        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectRect(Ref<UI::Composer> Composer, Text Label, Ref<Rect> Value)
    {
        Real32 MinimumX = Value.GetMinimumX();
        Real32 MinimumY = Value.GetMinimumY();
        Real32 MaximumX = Value.GetMaximumX();
        Real32 MaximumY = Value.GetMaximumY();

        Composer.PushID(Label);

        Composer.Field(String<64>::Print<"{0} (Minimum)">(Label));
        Bool Dirty = Composer.InputFloatPair("##minimum", MinimumX, MinimumY, "%.5f");

        Composer.Field(String<64>::Print<"{0} (Maximum)">(Label));
        Dirty |= Composer.InputFloatPair("##maximum", MaximumX, MaximumY, "%.5f");

        Composer.PopID();

        if (Dirty)
        {
            Value.Set(MinimumX, MinimumY, MaximumX, MaximumY);
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Callback>
    static void InspectPath(Ref<UI::Composer> Composer, Text Label, Text Value, AnyRef<Callback> Action)
    {
        // TODO: Swap the raw path field for the asset browser once it can be embedded as a picker.
        Composer.Field(Label);
        Composer.PushID(Label);
        Composer.SetNextItemWidth(-1.0f);
        Composer.InputText("##value", Value, Action, ImGuiInputTextFlags_EnterReturnsTrue);
        Composer.PopID();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static void Rescale(Ref<Context> Context, Scene::Entity Actor, ConstRef<Sprite> Component)
    {
        if (!Actor.IsValid())
        {
            return;
        }

        // Only an extent that has never been sized is derived from the sprite. Once it holds a size it belongs to
        // whoever authored it, and later edits to the source must leave it alone.
        const ConstPtr<Extent> Volume = Actor.TryGet<const Extent>();

        if (Volume && !Volume->GetSize().IsAlmostZero())
        {
            return;
        }

        ConstRetainer<Graphic::Material> Material = Context.GetContent().Load<Graphic::Material>(Component.GetPath());

        if (!Material || !Material->HasCompleted())
        {
            return;
        }

        ConstRetainer<Graphic::Image> Albedo = Material->GetImage(Graphic::TextureSlot::Albedo);

        if (!Albedo)
        {
            return;
        }

        // The source crop is normalized, so scaling it back up by the atlas gives the sprite's own pixels.
        const Rect   Source  = Component.GetSource();
        const Real32 Density = Context.GetDirector().GetDensity();

        const Vector2 Size(
            Source.GetWidth()  * Albedo->GetWidth()  / Density,
            Source.GetHeight() * Albedo->GetHeight() / Density);

        // Only the size is derived, so an offset the user already authored is carried over untouched.
        Actor.Set(Extent(Volume ? Volume->GetOffset() : Vector2::Zero(), Size));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Anchor> Component)
    {
        const Real32 Density = Context.GetDirector().GetDensity();

        Vector2 Value = Component.GetValue() * Density;

        if (InspectVector(Composer, "Value", Value))
        {
            Component.SetValue(Value / Density);
            return true;
        }
        return false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Extent> Component)
    {
        const Real32 Density = Context.GetDirector().GetDensity();

        Bool Dirty = false;

        if (Vector2 Offset = Component.GetOffset() * Density; InspectVector(Composer, "Offset", Offset))
        {
            Component.SetOffset(Offset / Density);
            Dirty = true;
        }

        Composer.Spacing();

        if (Vector2 Size = Component.GetSize() * Density; InspectVector(Composer, "Size", Size))
        {
            Component.SetSize(Size / Density);
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Pose> Component)
    {
        const Real32 Density = Context.GetDirector().GetDensity();

        Bool Dirty = false;

        if (Vector2 Translation = Component.GetTranslation() * Density; InspectVector(Composer, "Translation", Translation))
        {
            Component.SetTranslation(Translation / Density);
            Dirty = true;
        }

        Composer.Spacing();

        if (Vector2 Scale = Component.GetScale(); InspectVector(Composer, "Scale", Scale))
        {
            Component.SetScale(Scale);
            Dirty = true;
        }

        Composer.Spacing();

        if (Angle Rotation = Component.GetRotation(); InspectAngle(Composer, "Rotation", Rotation))
        {
            Component.SetRotation(Rotation);
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

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Velocity> Component)
    {
        Bool Dirty = false;

        if (Vector2 Linear = Component.GetLinear(); InspectVector(Composer, "Linear", Linear))
        {
            Component.SetLinear(Linear);
            Dirty = true;
        }

        Composer.Spacing();

        if (Angle Angular = Component.GetAngular(); InspectAngularRate(Composer, "Angular", Angular))
        {
            Component.SetAngular(Angular);
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Glowlight> Component)
    {
        Bool Dirty = false;

        if (Real32 Radius = Component.GetRadius(); InspectScalar(Composer, "Radius", Radius))
        {
            Component.SetRadius(Radius);
            Dirty = true;
        }

        Composer.Spacing();

        if (Real32 Intensity = Component.GetIntensity(); InspectScalar(Composer, "Intensity", Intensity))
        {
            Component.SetIntensity(Intensity);
            Dirty = true;
        }

        Composer.Spacing();

        if (Real32 Falloff = Component.GetFalloff(); InspectScalar(Composer, "Falloff", Falloff))
        {
            Component.SetFalloff(Falloff);
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Spotlight> Component)
    {
        Bool  Dirty = false;
        Angle Inner = Component.GetInnerAngle();
        Angle Outer = Component.GetOuterAngle();

        if (InspectAngle(Composer, "Inner Angle", Inner))
        {
            Component.SetAngles(Inner, Outer);
            Dirty = true;
        }

        Composer.Spacing();

        if (InspectAngle(Composer, "Outer Angle", Outer))
        {
            Component.SetAngles(Inner, Outer);
            Dirty = true;
        }

        Composer.Spacing();

        if (Real32 Range = Component.GetRange(); InspectScalar(Composer, "Range", Range))
        {
            Component.SetRange(Range);
            Dirty = true;
        }

        Composer.Spacing();

        if (Real32 Intensity = Component.GetIntensity(); InspectScalar(Composer, "Intensity", Intensity))
        {
            Component.SetIntensity(Intensity);
            Dirty = true;
        }

        Composer.Spacing();

        if (Real32 Falloff = Component.GetFalloff(); InspectScalar(Composer, "Falloff", Falloff))
        {
            Component.SetFalloff(Falloff);
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Skylight> Component)
    {
        Bool Dirty = false;

        if (Vector2 Direction = Component.GetSunDirection(); InspectVector(Composer, "Sun Direction", Direction))
        {
            Component.SetSunDirection(Direction);
            Dirty = true;
        }

        Composer.Spacing();

        if (IntColor8 Tint = Component.GetSunTint(); InspectTint(Composer, "Sun Tint", Tint))
        {
            Component.SetSunTint(Tint);
            Dirty = true;
        }

        Composer.Spacing();

        if (IntColor8 Tint = Component.GetSkyTint(); InspectTint(Composer, "Sky Tint", Tint))
        {
            Component.SetSkyTint(Tint);
            Dirty = true;
        }

        Composer.Spacing();

        if (IntColor8 Tint = Component.GetGroundTint(); InspectTint(Composer, "Ground Tint", Tint))
        {
            Component.SetGroundTint(Tint);
            Dirty = true;
        }

        Composer.Spacing();

        if (Real32 Brightness = Component.GetBrightness(); InspectScalar(Composer, "Brightness", Brightness))
        {
            Component.SetBrightness(Brightness);
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Sprite> Component)
    {
        Bool Dirty = false;

        InspectPath(Composer, "Path", Component.GetPath().GetUrl(), [&](Text Value)
        {
            Component.SetPath(Str::Print<"Resources://{0}">(Value));
            Dirty = true;
        });

        Composer.Spacing();

        if (Rect Source = Component.GetSource(); InspectRect(Composer, "Source", Source))
        {
            Component.SetSource(Source);
            Dirty = true;
        }

        if (Dirty)
        {
            Rescale(Context, Actor, Component);
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Animation> Component)
    {
        Bool Dirty = false;

        Composer.TextDisabled("{0} frame(s), {1:.2f}s total", Component.GetCount(), Component.GetDuration());
        Composer.Spacing();

        if (Composer.DisabledButton(ICON_FA_PLUS "  Add Frame", Component.IsFull(), -1.0f))
        {
            Component.Insert(Rect(0.0f, 0.0f, 1.0f, 1.0f), 0.1f);
            Dirty = true;
        }

        Composer.Spacing();

        if (Component.IsEmpty())
        {
            constexpr Text kHint = "No Frames";

            Composer.SetCursorPosX((Composer.GetContentRegionAvail().x - Composer.CalcTextSize(kHint).x) * 0.5f);
            Composer.TextDisabled(kHint);
            return Dirty;
        }

        constexpr ImGuiTableFlags kTableFlags =
            ImGuiTableFlags_BordersOuter  |
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_RowBg         |
            ImGuiTableFlags_SizingStretchSame;

        if (Composer.BeginTable("##frames", 7, kTableFlags))
        {
            Composer.TableSetupColumn("#",    ImGuiTableColumnFlags_WidthFixed,   18.0f);
            Composer.TableSetupColumn("X",    ImGuiTableColumnFlags_WidthStretch);
            Composer.TableSetupColumn("Y",    ImGuiTableColumnFlags_WidthStretch);
            Composer.TableSetupColumn("W",    ImGuiTableColumnFlags_WidthStretch);
            Composer.TableSetupColumn("H",    ImGuiTableColumnFlags_WidthStretch);
            Composer.TableSetupColumn("Time", ImGuiTableColumnFlags_WidthStretch);
            Composer.TableSetupColumn("",     ImGuiTableColumnFlags_WidthFixed,   16.0f);
            Composer.TableHeadersRow();

            UInt8 Discard = Animation::kMaxFrames;

            for (UInt8 Keyframe = 0; Keyframe < Component.GetCount(); ++Keyframe)
            {
                const Rect Data     = Component.GetFrameData(Keyframe);
                Real32     X        = Data.GetMinimumX();
                Real32     Y        = Data.GetMinimumY();
                Real32     Width    = Data.GetWidth();
                Real32     Height   = Data.GetHeight();
                Real32     Duration = Component.GetFrameDuration(Keyframe);

                Composer.PushID(Keyframe);
                Composer.TableNextRow();

                Composer.TableSetColumnIndex(0);
                Composer.Label("{0}", Keyframe + 1);

                const auto OnCell = [&](SInt32 Column, Text ID, Ref<Real32> Value, Text Format)
                {
                    Composer.TableSetColumnIndex(Column);
                    Composer.SetNextItemWidth(-1.0f);
                    return Composer.InputFloat(ID, Value, 0.0f, 0.0f, Format);
                };

                Bool Bounds = OnCell(1, "##x", X, "%.3f");
                Bounds |= OnCell(2, "##y", Y, "%.3f");
                Bounds |= OnCell(3, "##w", Width, "%.3f");
                Bounds |= OnCell(4, "##h", Height, "%.3f");

                if (Bounds)
                {
                    Component.SetFrameData(Keyframe, Rect(X, Y, X + Width, Y + Height));
                    Dirty = true;
                }

                if (OnCell(5, "##t", Duration, "%.2f"))
                {
                    Component.SetFrameDuration(Keyframe, Max(Duration, 0.01f));
                    Dirty = true;
                }

                Composer.TableSetColumnIndex(6);

                if (Composer.SmallButton(ICON_FA_XMARK))
                {
                    Discard = Keyframe;
                }

                Composer.PopID();
            }

            Composer.EndTable();

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

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Typeface> Component)
    {
        Bool Dirty = false;

        ConstRetainer<::Render::Font> Font = Component.GetFont();

        InspectPath(Composer, "Font", Font ? Font->GetKey().GetUrl() : Text(), [&](Text Value)
        {
            Component.SetFont(Str::Print<"Resources://{0}">(Value));
            Dirty = true;
        });

        Composer.Spacing();

        if (Real32 Size = Component.GetSize(); InspectScalar(Composer, "Size", Size))
        {
            Component.SetSize(Size);
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Label> Component)
    {
        Bool Dirty = false;

        Composer.Field("Content");
        Composer.PushID("Content");
        Composer.SetNextItemWidth(-1.0f);
        Composer.InputText("##value", Component.GetContent(), [&](Text Value)
        {
            Component.SetContent(Value);
            Dirty = true;
        });
        Composer.PopID();

        Composer.Spacing();

        if (Vector2 Spacing = Component.GetSpacing(); InspectVector(Composer, "Spacing", Spacing))
        {
            Component.SetSpacing(Spacing);
            Dirty = true;
        }

        Composer.Spacing();

        const Pivot2D Pivot = Component.GetPivot();

        if (Vector2 Value(Pivot.GetX(), Pivot.GetY()); InspectVector(Composer, "Pivot", Value))
        {
            Component.SetPivot(Pivot2D(Value.GetX(), Value.GetY()));
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<IntColor8> Component)
    {
        return InspectTint(Composer, "Color", Component);
    }
}
