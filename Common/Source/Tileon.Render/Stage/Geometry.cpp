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

#include "Geometry.hpp"
#include <Tileon.Render/Types.hpp>
#include "Tileon.Render/Component.hpp"
#include "Tileon.World/Component.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Stage
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Vector3 Measure(ConstRef<Appearance> Appearance, Real32 Density)
    {
        const Vector2 Size = Appearance.GetSource().GetSize() * Appearance.GetResolution() / Density;

        switch (Appearance.GetPlane())
        {
        case Sprite::Plane::Ground:
            return Vector3(Size.GetX(), 0.0f, Size.GetY());
        case Sprite::Plane::Wall:
            return Vector3(0.0f, Size.GetX(), Size.GetY());
        default:
            return Vector3(Size.GetX(), Size.GetY(), 0.0f);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Vector2 Span(Vector3 Size, Sprite::Plane Plane)
    {
        switch (Plane)
        {
        case Sprite::Plane::Ground:
            return Size.GetXZ();
        case Sprite::Plane::Wall:
            return Vector2(Size.GetY(), Size.GetZ());
        default:
            return Size.GetXY();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Geometry::Geometry(Ref<Engine::Subsystem::Host> Host, Ref<Splatset> Splatset, Real32 Density)
        : Locator    { Host },
          mSplatset  { Splatset },
          mDensity   { Density },
          mDirector  { nullptr },
          mSprites   { Host.GetService<Graphic::Service>(), mCollector, Enum::Cast(Order::Sprite) },
          mGlyphs    { Host.GetService<Graphic::Service>(), mCollector, Enum::Cast(Order::Glyph) },
          mSplatter  { Host.GetService<Graphic::Service>(), Splatset, Density }
    {
        OnRegister(* Host.GetService<Scene::Service>());
        OnLoad(* Host.GetService<Content::Service>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Geometry::Run(Ref<Render::Encoder> Encoder)
    {
        ZY_PROFILE_SCOPE("Stage::Geometry::Run");

        const IntVector3 Origin = mDirector->GetOrigin();

        mCollector.Begin(Render::Collector::Priority::Opaque);
        {
            ZY_PROFILE_SCOPE("Stage::Geometry::Opaque");

            // -- Opaque Sprites --
            mSprites.Reset();
            mSprites.SetPriority(Render::Collector::Priority::Opaque);

            mSprites.SetTechnique(
                mTechniques[Enum::Cast(Kind::Sprite)],
                mTechniques[Enum::Cast(Kind::Sprite)]->ResolveByName("Cutout"));
            mQrDrawOpaque.Run<>([&](
                ConstRef<Transform>  Transform,
                ConstRef<Extent>     Extent,
                ConstRef<Enclosure>  Enclosure,
                ConstRef<Appearance> Appearance,
                ConstPtr<IntColor8>  Tint)
            {
                if (mDirector->IsVisible(Enclosure.GetVolume()))
                {
                    const Matrix4x3 Matrix = Transform.Rebase(Origin);

                    mSprites.Draw(
                        Appearance,
                        Span(Extent.GetSize(), Appearance.GetPlane()),
                        Matrix,
                        Tint ? (* Tint) : IntColor8::White());
                }
            });
        }
        Drain(Encoder);

        // -- Splatmap --
        {
            ZY_PROFILE_SCOPE("Stage::Geometry::Ground");

            ConstRetainer<Graphic::Technique> Technique = mTechniques[Enum::Cast(Kind::Splat)];

            const IntRect Frustum = mDirector->GetFrustum();

            mQrDrawRegions.Run<>([&](ConstRef<Region> Region, Ref<Splatmap> Splat)
            {
                const SInt32 RegionX = Region.GetX() * Region::kUnitsPerX;
                const SInt32 RegionY = Region.GetY() * Region::kUnitsPerY;

                const IntRect Boundaries(RegionX, RegionY, RegionX + Region::kUnitsPerX, RegionY + Region::kUnitsPerY);
                mSplatter.Record(Region, Splat, !IntRect::Intersection(Boundaries, Frustum).IsAlmostZero());
            });
            mSplatter.Draw(Encoder, Technique, Origin);
        }

        // -- Transparent --
        mCollector.Begin(Render::Collector::Priority::Transparent);
        {
            ZY_PROFILE_SCOPE("Stage::Geometry::Transparent");

            // -- Sprites --
            mSprites.Reset();
            mSprites.SetPriority(Render::Collector::Priority::Transparent);
            mSprites.SetTechnique(mTechniques[Enum::Cast(Kind::Sprite)], 0);

            mQrDrawTransparent.Run<>([&](
                ConstRef<Transform>  Transform,
                ConstRef<Extent>     Extent,
                ConstRef<Enclosure>  Enclosure,
                ConstRef<Appearance> Appearance,
                ConstPtr<IntColor8>  Tint)
            {
                if (mDirector->IsVisible(Enclosure.GetVolume()))
                {
                    const Matrix4x3 Matrix = Transform.Rebase(Origin);

                    mSprites.Draw(
                        Appearance,
                        Span(Extent.GetSize(), Appearance.GetPlane()),
                        Matrix,
                        Tint ? (* Tint) : IntColor8::White());
                }
            });

            // -- Glyphs --
            mGlyphs.Reset();
            mGlyphs.SetTechnique(mTechniques[Enum::Cast(Kind::Text)]);

            mQrDrawTexts.Run<>([&](
                ConstRef<Transform>  Transform,
                ConstRef<Enclosure>  Enclosure,
                ConstRef<Typeface>   Face,
                ConstRef<Label>      Label,
                ConstPtr<IntColor8>  Tint,
                ConstPtr<Contour>    Effect)
            {
                if (mDirector->IsVisible(Enclosure.GetVolume()))
                {
                    const Matrix4x3 Matrix = Transform.Rebase(Origin);

                    mGlyphs.Draw(
                        Face,
                        Label,
                        Matrix,
                        Effect ? * Effect : Contour(),
                        Tint ? (* Tint) : IntColor8::White());
                }
            });
        }
        Drain(Encoder);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Geometry::Drain(Ref<Render::Encoder> Encoder)
    {
        ZY_PROFILE_SCOPE("Stage::Geometry::Drain");

        // The runs the glyph batches index into are uploaded here, so a drain can never read a stale palette.
        mGlyphs.Prepare();

        // Sprites and text share the queue, so one sorted drain hands each batch back to whichever recorded it.
        mCollector.Poll([&](UInt32 Kind, ConstSpan<Render::Collector::Command> Commands)
        {
            switch (static_cast<Order>(Kind))
            {
            case Order::Sprite:
                mSprites.Write(Encoder, Commands);
                break;
            case Order::Glyph:
                mGlyphs.Write(Encoder, Commands);
                break;
            }
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Geometry::OnRegister(Ref<Scene::Service> Scene)
    {
        Scene.Register(
            Scene::DSL::Declare<Animator, Appearance>(),
            Scene::DSL::Declare<Splatmap>(Scene::DSL::Serializable),
            Scene::DSL::Declare<IntColor8>("Tint", Scene::DSL::Authored),
            Scene::DSL::Declare<Transparent>(Scene::DSL::Authored),
            Scene::DSL::Declare<Animation, Contour, Label, Typeface, Sprite>(Scene::DSL::Authored));

        // Observe when a lettering component is attached, and automatically provide the label it sets the text of.
        Scene.CreateObserver<Scene::DSL::With<Typeface>>(
            "Render::Geometry::ObsAttachLabelWithTypeface",
            EcsOnAdd,
            [](Scene::Entity Actor)
            {
                Actor.Emplace<Label>();
            });

        // A region that streams out takes its splat with it, so the slice it held is given back.
        Scene.CreateObserver<>(
            "Render::Geometry::ObsReleaseSplatmap",
            EcsOnRemove,
            [this](Scene::Entity Actor, Ref<Splatmap> Splat)
            {
                mSplatter.Release(Splat);
            });

        // Observe changes to the region component to invalidate its ground.
        Scene.CreateObserver<>(
            "Render::Geometry::ObsInvalidateSplatmap",
            EcsOnSet,
            [](Scene::Entity Actor, Ref<Splatmap> Splat)
            {
                Splat.Invalidate();
            });

        // Observe changes to the sprite component to resolve material resources and trigger updates when necessary.
        Scene.CreateObserver<Scene::DSL::InOut<Sprite>>(
            "Render::Geometry::ObsUpdateAppearanceOnSpriteUpdate",
            EcsOnSet,
            [this](Scene::Entity Actor, ConstRef<Sprite> Component)
            {
                Ref<Content::Service> Content = GetService<Content::Service>();

                if (ConstRetainer<Graphic::Material> Material = Content.Load<Graphic::Material>(Component.GetPath()))
                {
                    if (!Material->HasFinished())
                    {
                        Content::Service::Callback Callback = [this, Actor](Ref<Content::Resource> Resource)
                        {
                            GetService<Content::Service>().Unsubscribe(Resource.GetKey());

                            if (Actor.IsAlive())
                            {
                                Actor.Notify<Sprite>();
                            }
                        };
                        Content.Subscribe(Material->GetKey(), Move(Callback));
                    }
                    else
                    {
                        Vector2 Resolution = Vector2::Zero();

                        if (Material->HasCompleted())
                        {
                            if (ConstRetainer<Graphic::Image> Albedo = Material->GetImage(GetTextureID(Texture::Albedo)))
                            {
                                Resolution = Vector2(Albedo->GetWidth(), Albedo->GetHeight());
                            }
                        }

                        Appearance Appearance(
                            Material,
                            Component.GetSource(),
                            Resolution,
                            Component.GetFacing(),
                            Component.GetPlane());

                        Actor.Set(Extent(Vector3::Zero(), Measure(Appearance, mDensity)));
                        Actor.Set(Move(Appearance));
                    }
                }
            }, Scene::DSL::Opt(EcsPrefab));

        // Observe when an animation component is attached, and automatically initialize the animator for playback.
        // TODO: See if this should be like this
        Scene.CreateObserver<>(
            "Render::Geometry::ObsUpdateAnimatorOnAnimationUpdate",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Animation> Animation)
            {
                Actor.Emplace<Animator>(0.0, Animator::Status::Repeat);
            });

        // Observe changes to the lettering component to resolve font resources and trigger updates when necessary.
        Scene.CreateObserver<Scene::DSL::InOut<Typeface>>(
            "Render::Geometry::ObsUpdateTypefaceAsync",
            EcsOnSet,
            [this](Scene::Entity Actor, Ref<Typeface> Component)
            {
                Ref<Content::Service> Content = GetService<Content::Service>();
                Component.OnResolve(Content);

                if (ConstRetainer<::Render::Font> Font = Component.GetFont(); Font && !Font->HasFinished())
                {
                    Content::Service::Callback Callback = [&Content, Actor](Ref<Content::Resource> Resource)
                    {
                        Content.Unsubscribe(Resource.GetKey());

                        Actor.Notify<Typeface>();
                    };
                    Content.Subscribe(Font->GetKey(), Move(Callback));
                }
            }, Scene::DSL::In(EcsPrefab));

        // Observe changes to the lettering or label components to update the dimension and origin of text entities when necessary.
        Scene.CreateObserver<>(
            "Render::Geometry::ObsUpdateTextBoundaries",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Typeface> Face, ConstRef<Label> Label)
            {
                if (ConstRetainer<::Render::Font> Font = Face.GetFont(); Font && Font->HasFinished())
                {
                    const Pivot2D Pivot   = Label.GetPivot();
                    const Rect    AABB    = Font->Enclose(Label.GetContent(), Face.GetSize(), Label.GetSpacing());
                    const Vector2 Measure = AABB.GetSize();

                    Actor.Set(Extent(Vector3::FromXY(AABB.GetPosition()), Vector3::FromXY(Measure)));
                    Actor.Set(Anchor(Vector3(Measure.GetX() * Pivot.GetX(), 0.0f, Measure.GetY() * Pivot.GetY())));
                }
            });

        // System that advances animations and updates sprite appearances.
        Scene.CreateSystem<>(
            "Render::Geometry::ComputeAnimation",
            EcsOnUpdate,
            Scene::Execution::Default,
            [this](Scene::Entity          Actor,
                   ConstRef<Scene::Clock> Clock,
                   ConstRef<Animation>    Animation,
                   Ref<Animator>          Animator,
                   Ref<Appearance>        Appearance,
                   Ref<Extent>            Extent)
            {
                // Advance the animator's timestamp and update the current keyframe based on the elapsed time.
                Animator.Advance(Clock.GetAbsolute(), Animation);

                // A keyframe cropping the same rectangle as the one before it leaves the quad alone, so a
                // sprite whose frames share a size measures once and never again.
                const Rect Source = Animation.GetFlipbook().GetData(Animator.GetKeyframe());

                if (Source != Appearance.GetSource())
                {
                    Appearance.SetSource(Source);

                    Extent.SetSize(Measure(Appearance, mDensity));
                }
            });

        mQrDrawOpaque = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Extent, const Enclosure, const Appearance, ConstPtr<IntColor8>>,
            Scene::DSL::Not<Transparent>
        >("Render::Geometry::DrawOpaque", Scene::Cache::Auto);

        mQrDrawTransparent = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Extent, const Enclosure, const Appearance, ConstPtr<IntColor8>>,
            Scene::DSL::With<Transparent>
        >("Render::Geometry::DrawTransparent", Scene::Cache::Auto);

        mQrDrawTexts = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Enclosure, const Typeface, const Label, ConstPtr<IntColor8>, ConstPtr<Contour>>
        >("Render::Geometry::DrawTexts", Scene::Cache::Auto);

        mQrDrawRegions = Scene.CreateQuery<
            Scene::DSL::In<Region>, Scene::DSL::InOut<Splatmap>
        >("Render::Geometry::DrawRegions", Scene::Cache::Auto);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Geometry::OnLoad(Ref<Content::Service> Content)
    {
        for (const Kind Type : Enum::GetValues<Kind>())
        {
            Str Path = Str::Print<"Resources://Technique/Geometry/{0}.vfx">(Enum::GetName(Type));

            mTechniques[Enum::Cast(Type)] = Content.Load<Graphic::Technique>(Move(Path));
        }
    }
}