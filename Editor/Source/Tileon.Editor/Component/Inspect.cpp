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

    static Bool InspectVector(
        Ref<UI::Composer> Composer,
        Text              Label,
        Ref<Vector2>      Value,
        Real32            Speed = 1.0f,
        Real32            Min   = 0.0f,
        Real32            Max   = 0.0f)
    {
        Real32 X = Value.GetX();
        Real32 Y = Value.GetY();

        Composer.FieldInline(Label);
        Composer.PushID(Label);

        const Bool Dirty = Composer.InputFloatPair("##value", X, Y, "%.3f", Speed, Min, Max);

        Composer.PopID();

        if (Dirty)
        {
            Value.Set(X, Y);
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectScalar(Ref<UI::Composer> Composer, Text Label, Ref<Real32> Value, Real32 Speed = 0.1f)
    {
        Composer.FieldInline(Label);
        Composer.PushID(Label);

        const Bool Dirty = Composer.DragFloat("##value", Value, Speed);

        Composer.PopID();

        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectAngle(Ref<UI::Composer> Composer, Text Label, Ref<Angle> Value, Real32 Min = 0.0f, Real32 Max = 360.0f)
    {
        Real32 Radians = Value.GetRadians();

        Composer.FieldInline(Label);
        Composer.PushID(Label);

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

        Composer.FieldInline(Label);
        Composer.PushID(Label);

        const Bool Dirty = Composer.DragFloat("##value", Degrees, 0.1f, 0.0f, 0.0f, "%.2f°/s");

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
        Composer.FieldInline(Label);
        Composer.PushID(Label);

        const Bool Dirty = Composer.InputTintSmall("##value", Value);

        Composer.PopID();

        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectRect(
        Ref<UI::Composer> Composer,
        Text              Label,
        Ref<Rect>         Value,
        Real32            ScaleX = 1.0f,
        Real32            ScaleY = 1.0f,
        Real32            Speed  = 1.0f)
    {
        Real32 MinimumX = Value.GetMinimumX() * ScaleX;
        Real32 MinimumY = Value.GetMinimumY() * ScaleY;
        Real32 MaximumX = Value.GetMaximumX() * ScaleX;
        Real32 MaximumY = Value.GetMaximumY() * ScaleY;

        Composer.PushID(Label);

        Composer.Field(String<64>::Print<"{0} (Min)">(Label));
        Bool Dirty = Composer.InputFloatPair("##minimum", MinimumX, MinimumY, "%.2f", Speed);

        Composer.Field(String<64>::Print<"{0} (Max)">(Label));
        Dirty |= Composer.InputFloatPair("##maximum", MaximumX, MaximumY, "%.2f", Speed);

        Composer.PopID();

        if (Dirty)
        {
            Value.Set(MinimumX / ScaleX, MinimumY / ScaleY, MaximumX / ScaleX, MaximumY / ScaleY);
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool InspectPivot(Ref<UI::Composer> Composer, Text Label, Ref<Pivot2D> Value)
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

        Composer.FieldInline(Label);
        Composer.PushID(Label);

        ConstPtr<Preset> Match = nullptr;

        for (ConstRef<Preset> Option : kPresets)
        {
            if (Matches(Option))
            {
                Match = AddressOf(Option);
                break;
            }
        }

        Bool Custom = !Match || Composer.GetState("custom");
        Bool Dirty  = false;

        if (Composer.BeginCombo("##value", Custom ? kCustom : Match->Name))
        {
            for (ConstRef<Preset> Option : kPresets)
            {
                if (Composer.Selectable(Option.Name, !Custom && Matches(Option)))
                {
                    Value  = Option.Pivot;
                    Custom = false;
                    Dirty  = true;
                }
            }

            if (Composer.Selectable(kCustom, Custom))
            {
                Custom = true;
            }
            Composer.EndCombo();
        }

        Composer.SetState("custom", Custom);

        if (Custom)
        {
            Real32 X = Value.GetX();
            Real32 Y = Value.GetY();

            if (Composer.InputFloatPair("##custom", X, Y, "%.3f", 0.01f))
            {
                Value = Pivot2D(X, Y);
                Dirty = true;
            }
        }

        Composer.PopID();

        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    template<typename Callback>
    static void InspectAsset(
        Ref<UI::Composer>      Composer,
        Ref<Workspace>         Workspace,
        Scene::Entity          Actor,
        Text                   Label,
        Text                   Filter,
        ConstRef<Content::Uri> Value,
        AnyRef<Callback>       Action)
    {
        Ref<UI::Selector> Selector = Workspace.Selector;

        const UInt64 Key = Composer.IsDisabled() ? 0 : HashCombine(Actor.GetID(), Label);

        if (Str Selection; Selector.Consume(Key, Selection))
        {
            Action(Content::Uri(Move(Selection)));
        }

        Composer.FieldInline(Label);
        Composer.PushID(Label);
        Composer.InputTextWithButton("##value", Value.GetPath(),
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

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Anchor> Component)
    {
        const Real32 Density = Workspace.Context.GetDirector().GetDensity();

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

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Extent> Component)
    {
        const Real32 Density = Workspace.Context.GetDirector().GetDensity();

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

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Pose> Component)
    {
        const Real32 Density = Workspace.Context.GetDirector().GetDensity();

        Bool Dirty = false;

        Vector2 Base = Vector2::Zero();

        if (const ConstPtr<Tileon::Transform> Transform = Actor.TryGet<const Tileon::Transform>())
        {
            Base = Vector2(Transform->GetOrigin());
        }

        if (Vector2 Translation = (Component.GetTranslation() + Base) * Density; InspectVector(Composer, "Translation", Translation))
        {
            Component.SetTranslation(Translation / Density - Base);
            Dirty = true;
        }

        Composer.Spacing();

        if (Vector2 Scale = Component.GetScale(); InspectVector(Composer, "Scale", Scale, 0.01f, 0.0f, FLT_MAX))
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

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Velocity> Component)
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

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Glowlight> Component)
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

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Spotlight> Component)
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

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Skylight> Component)
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

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Sprite> Component)
    {
        Bool Dirty = false;

        InspectAsset(Composer, Workspace, Actor, "Path", ".mtl", Component.GetPath(),
            [&](AnyRef<Content::Uri> Path)
            {
                Component.SetPath(Move(Path));
                Dirty = true;
            });

        Composer.Spacing();

        Real32 ScaleX = 1.0f;
        Real32 ScaleY = 1.0f;

        ConstRetainer<Graphic::Material> Material = Workspace.Context.GetContent().Load<Graphic::Material>(Component.GetPath());

        if (Material && Material->HasCompleted())
        {
            if (ConstRetainer<Graphic::Image> Albedo = Material->GetImage(Graphic::TextureSlot::Albedo))
            {
                ScaleX = Albedo->GetWidth();
                ScaleY = Albedo->GetHeight();
            }
        }

        if (Rect Source = Component.GetSource(); InspectRect(Composer, "Source", Source, ScaleX, ScaleY))
        {
            Component.SetSource(Source);
            Dirty = true;
        }

        if (Dirty)
        {
            Rescale(Workspace.Context, Actor, Component);
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Animation> Component)
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

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Typeface> Component)
    {
        const Real32 Density = Workspace.Context.GetDirector().GetDensity();

        Bool Dirty = false;

        ConstRetainer<::Render::Font> Font = Component.GetFont();

        InspectAsset(Composer, Workspace, Actor, "Font", ".artery", Font ? Font->GetKey() : Content::Uri(),
            [&](AnyRef<Content::Uri> Path)
            {
                Component.SetFont(Move(Path));
                Dirty = true;
            });

        Composer.Spacing();

        if (Real32 Size = Component.GetSize() * Density; InspectScalar(Composer, "Size", Size))
        {
            Component.SetSize(Size / Density);
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Label> Component)
    {
        const Real32 Density = Workspace.Context.GetDirector().GetDensity();

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

        if (Vector2 Spacing = Component.GetSpacing() * Density; InspectVector(Composer, "Spacing", Spacing))
        {
            Component.SetSpacing(Spacing / Density);
            Dirty = true;
        }

        Composer.Spacing();

        if (Pivot2D Pivot = Component.GetPivot(); InspectPivot(Composer, "Pivot", Pivot))
        {
            Component.SetPivot(Pivot);
            Dirty = true;
        }
        return Dirty;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ref<IntColor8> Component)
    {
        return InspectTint(Composer, "Color", Component);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool Inspect(Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Emphasis> Component)
    {
        ConstRef<Render::TextEffect> Effect = Component.GetEffect();

        Color  OutsetColor     = Effect.GetOutsetColor();
        Real32 OutsetOffset    = Effect.GetOutsetOffset();
        Real32 OutsetWidth     = Effect.GetOutsetWidthRelative();
        Real32 OutsetBias      = Effect.GetOutsetWidthAbsolute();
        Real32 OutsetBlur      = Effect.GetOutsetBlur();
        Real32 InsetRoundness  = Effect.GetInsetRoundness();
        Real32 InsetThreshold  = Effect.GetInsetThreshold();

        Bool Dirty = false;

        Composer.FieldInline("Outset Color");
        Composer.PushID("Outset Color");
        Dirty |= Composer.InputTintSmall("##value", OutsetColor);
        Composer.PopID();

        Dirty |= InspectScalar(Composer, "Outset Offset",    OutsetOffset);
        Dirty |= InspectScalar(Composer, "Outset Width",     OutsetWidth);
        Dirty |= InspectScalar(Composer, "Outset Bias",      OutsetBias);
        Dirty |= InspectScalar(Composer, "Outset Blur",      OutsetBlur);
        Dirty |= InspectScalar(Composer, "Inset Roundness",  InsetRoundness);
        Dirty |= InspectScalar(Composer, "Inset Threshold",  InsetThreshold);

        if (Dirty)
        {
            Component.SetEffect(Render::TextEffect(
                OutsetColor, OutsetOffset, OutsetWidth, OutsetBias, OutsetBlur, InsetRoundness, InsetThreshold));
        }
        return Dirty;
    }
}
