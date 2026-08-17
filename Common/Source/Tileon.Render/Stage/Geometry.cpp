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
#include "Tileon.Render/Component.hpp"
#include "Tileon.World/Component.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Stage
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Vector3 Measure(Ref<Appearance> Appearance, Real32 Density, Bool Grounded)
    {
        const Rect    Source     = Appearance.GetSource();
        const Vector2 Resolution = Appearance.GetResolution();

        const Real32  Width      = Source.GetWidth()  * Resolution.GetX() / Density;
        const Real32  Height     = Source.GetHeight() * Resolution.GetY() / Density;

        // A decal's quad spans the local x and z axes, so its box has to lie the same way or the enclosure
        // it drives would stand upright and pick nothing that the eye sees.
        return Grounded ? Vector3(Width, 0.0f, Height) : Vector3(Width, Height, 0.0f);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Geometry::Geometry(Ref<Engine::Subsystem::Host> Host, Real32 Density)
        : Locator   { Host },
          mDirector { nullptr },
          mDensity  { Density },
          mCutout   { 0 },
          mSprites  { Host.GetService<Graphic::Service>(), mCollector },
          mGlyphs   { Host.GetService<Graphic::Service>(), mCollector }
    {
        OnRegister(* Host.GetService<Scene::Service>());
        OnLoad(* Host.GetService<Content::Service>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Geometry::Run(Ref<Render::Encoder> Encoder)
    {
        ZY_PROFILE_SCOPE("Stage::Geometry::Run");

        const IntVector3 Origin  = IntVector3::FromXZ(
            IntVector2(mDirector->GetPosition().GetBaseX(),
                       mDirector->GetPosition().GetBaseY()));

        // Opaque entities go down next, so the ground behind them is rejected by depth rather than shaded twice.
        mCollector.Begin(Render::Collector::Priority::Opaque);

        {
            ZY_PROFILE_SCOPE("Stage::Geometry::Opaque");

            mSprites.Reset();

            // Draw opaque sprite entities.
            mSprites.SetTechnique(mTechniques[Enum::Cast(Kind::Sprite)], mCutout, Render::Collector::Priority::Opaque);
            mQrDrawOpaqueSprites.Run<>([&](
                ConstRef<Transform>  Transform,
                ConstRef<Extent>     Extent,
                ConstRef<Enclosure>  Enclosure,
                ConstRef<Appearance> Appearance,
                ConstPtr<IntColor8>  Tint)
            {
                if (mDirector->IsVisible(Enclosure.GetVolume()))
                {
                    const Matrix4x3 Matrix = Transform.Rebase(Origin);

                    mSprites.Draw(Appearance, Extent.GetSize().GetXY(), Matrix, Tint ? (* Tint) : IntColor8::White());
                }
            });

            // Decals ride the same batch as sprites and differ only in technique, which lays their quad against
            // the ground and biases its depth so it wins over the terrain it is painted on. Both variants stay
            // in the g-buffer so the lighting that follows reaches them; only the blend differs.
            mSprites.SetTechnique(mTechniques[Enum::Cast(Kind::Decal)], mCutout, Render::Collector::Priority::Opaque);
            mQrDrawOpaqueDecals.Run<>([&](
                ConstRef<Transform>  Transform,
                ConstRef<Extent>     Extent,
                ConstRef<Enclosure>  Enclosure,
                ConstRef<Appearance> Appearance,
                ConstPtr<IntColor8>  Tint)
            {
                if (mDirector->IsVisible(Enclosure.GetVolume()))
                {
                    const Matrix4x3 Matrix = Transform.Rebase(Origin);

                    mSprites.Draw(Appearance, Extent.GetSize().GetXZ(), Matrix, Tint ? (* Tint) : IntColor8::White());
                }
            });

            // A decal marked Transparent feathers into what it is painted on instead of cutting out.
            mSprites.SetTechnique(mTechniques[Enum::Cast(Kind::Decal)], 0, Render::Collector::Priority::Opaque);
            mQrDrawTransparentDecals.Run<>([&](
                ConstRef<Transform>  Transform,
                ConstRef<Extent>     Extent,
                ConstRef<Enclosure>  Enclosure,
                ConstRef<Appearance> Appearance,
                ConstPtr<IntColor8>  Tint)
            {
                if (mDirector->IsVisible(Enclosure.GetVolume()))
                {
                    const Matrix4x3 Matrix = Transform.Rebase(Origin);

                    mSprites.Draw(Appearance, Extent.GetSize().GetXZ(), Matrix, Tint ? (* Tint) : IntColor8::White());
                }
            });
        }
        Drain(Encoder);

        // Everything that blends comes last, drained back to front by the collector.
        mCollector.Begin(Render::Collector::Priority::Transparent);
        {
            ZY_PROFILE_SCOPE("Stage::Geometry::Transparent");

            mSprites.Reset();

            // Draw transparent sprite entities, each lit by the normal map its own material carries.
            mSprites.SetTechnique(mTechniques[Enum::Cast(Kind::Sprite)], 0, Render::Collector::Priority::Transparent);
            mQrDrawTransparentSprites.Run<>([&](
                ConstRef<Transform>  Transform,
                ConstRef<Extent>     Extent,
                ConstRef<Enclosure>  Enclosure,
                ConstRef<Appearance> Appearance,
                ConstPtr<IntColor8>  Tint)
            {
                if (mDirector->IsVisible(Enclosure.GetVolume()))
                {
                    const Matrix4x3 Matrix = Transform.Rebase(Origin);

                    mSprites.Draw(Appearance, Extent.GetSize().GetXY(), Matrix, Tint ? (* Tint) : IntColor8::White());
                }
            });

            mGlyphs.Reset();

            // Draw text entities.
            mGlyphs.SetTechnique(mTechniques[Enum::Cast(Kind::Text)]);
            mQrDrawTexts.Run<>([&](
                ConstRef<Transform>  Transform,
                ConstRef<Enclosure>  Enclosure,
                ConstRef<Lettering>  Lettering,
                ConstRef<Label>      Label,
                ConstPtr<IntColor8>  Tint,
                ConstPtr<Decoration> Decoration)
            {
                if (mDirector->IsVisible(Enclosure.GetVolume()))
                {
                    const Matrix4x3 Matrix = Transform.Rebase(Origin);

                    mGlyphs.Draw(
                        Lettering,
                        Label,
                        Matrix,
                        Decoration ? * Decoration : Tileon::Decoration(),
                        Tint ? (* Tint) : IntColor8::White());
                }
            });
        }
        Drain(Encoder);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


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
            switch (static_cast<Batcher::Batch>(Kind))
            {
            case Batcher::Batch::Sprite:
                mSprites.Write(Encoder, Commands);
                break;
            case Batcher::Batch::Glyph:
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
            Scene::DSL::Declare<IntColor8>("Tint", Scene::DSL::Authored),
            Scene::DSL::Declare<Transparent>(Scene::DSL::Authored),
            Scene::DSL::Declare<Decal>(Scene::DSL::Authored),
            Scene::DSL::Declare<Animation, Decoration, Label, Lettering, Sprite>(Scene::DSL::Authored));

        // Observe when a lettering component is attached, and automatically provide the label it sets the text of.
        Scene.CreateObserver<Scene::DSL::With<Lettering>>(
            "Render::Geometry::ObsAttachLabelWithLettering",
            EcsOnAdd,
            [](Scene::Entity Actor)
            {
                Actor.Emplace<Label>();
            });

        // A decal's quad spans a different pair of axes than a sprite's, so the box has to be measured again
        // when the tag arrives on art that was already resolved. Without this the box keeps the axes it was
        // measured on and the draw reads a zero along one of them, which is a quad with no area.
        Scene.CreateObserver<Scene::DSL::With<Decal>>(
            "Render::Geometry::ObsRemeasureOnDecalAttach",
            EcsOnAdd,
            [this](Scene::Entity Actor)
            {
                if (Ptr<Appearance> Visual = Actor.TryGet<Appearance>())
                {
                    Actor.Set(Extent(Vector3::Zero(), Measure(* Visual, mDensity, true)));
                }
            });

        // The same holds in reverse, so art that stops being a decal stands its box back up.
        Scene.CreateObserver<Scene::DSL::With<Decal>>(
            "Render::Geometry::ObsRemeasureOnDecalDetach",
            EcsOnRemove,
            [this](Scene::Entity Actor)
            {
                if (Ptr<Appearance> Visual = Actor.TryGet<Appearance>())
                {
                    Actor.Set(Extent(Vector3::Zero(), Measure(* Visual, mDensity, false)));
                }
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
                            if (ConstRetainer<Graphic::Image> Albedo = Material->GetImage("Albedo"_Hash))
                            {
                                Resolution = Vector2(Albedo->GetWidth(), Albedo->GetHeight());
                            }
                        }

                        Appearance Appearance(Material, Component.GetSource(), Resolution, Component.GetFacing());

                        Actor.Set(Extent(Vector3::Zero(), Measure(Appearance, mDensity, Actor.Has<Decal>())));
                        Actor.Set(Move(Appearance));
                    }
                }
            }, Scene::DSL::Opt(EcsPrefab));

        // Observe when an animation component is attached, and automatically initialize the animator for playback.
        Scene.CreateObserver<>(
            "Render::Geometry::ObsUpdateAnimatorOnAnimationUpdate",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Animation> Animation)
            {
                Actor.Emplace<Animator>(0.0, Animator::Status::Repeat);
            });

        // Observe changes to the lettering component to resolve font resources and trigger updates when necessary.
        Scene.CreateObserver<Scene::DSL::InOut<Lettering>>(
            "Render::Geometry::ObsUpdateLetteringAsync",
            EcsOnSet,
            [this](Scene::Entity Actor, Ref<Lettering> Component)
            {
                Ref<Content::Service> Content = GetService<Content::Service>();
                Component.OnResolve(Content);

                if (ConstRetainer<::Render::Font> Font = Component.GetFont(); Font && !Font->HasFinished())
                {
                    Content::Service::Callback Callback = [&Content, Actor](Ref<Content::Resource> Resource)
                    {
                        Content.Unsubscribe(Resource.GetKey());

                        Actor.Notify<Lettering>();
                    };
                    Content.Subscribe(Font->GetKey(), Move(Callback));
                }
            }, Scene::DSL::In(EcsPrefab));

        // Observe changes to the lettering or label components to update the dimension and origin of text entities when necessary.
        Scene.CreateObserver<>(
            "Render::Geometry::ObsUpdateTextBoundaries",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Lettering> Lettering, ConstRef<Label> Label)
            {
                if (ConstRetainer<::Render::Font> Font = Lettering.GetFont(); Font && Font->HasFinished())
                {
                    const Pivot2D Pivot   = Label.GetPivot();
                    const Rect    AABB    = Font->Enclose(Label.GetContent(), Lettering.GetSize(), Label.GetSpacing());
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
            [this](Scene::Entity           Actor,
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

                    Extent.SetSize(Measure(Appearance, mDensity, Actor.Has<Decal>()));
                }
            });

        mQrDrawOpaqueSprites = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Extent, const Enclosure, const Appearance, ConstPtr<IntColor8>>,
            Scene::DSL::Not<Transparent>, Scene::DSL::Not<Decal>
        >("Render::Geometry::DrawOpaqueSprites", Scene::Cache::Auto);

        mQrDrawTransparentSprites = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Extent, const Enclosure, const Appearance, ConstPtr<IntColor8>>,
            Scene::DSL::With<Transparent>, Scene::DSL::Not<Decal>
        >("Render::Geometry::DrawTransparentSprites", Scene::Cache::Auto);

        mQrDrawOpaqueDecals = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Extent, const Enclosure, const Appearance, ConstPtr<IntColor8>>,
            Scene::DSL::With<Decal>, Scene::DSL::Not<Transparent>
        >("Render::Geometry::DrawOpaqueDecals", Scene::Cache::Auto);

        mQrDrawTransparentDecals = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Extent, const Enclosure, const Appearance, ConstPtr<IntColor8>>,
            Scene::DSL::With<Decal>, Scene::DSL::With<Transparent>
        >("Render::Geometry::DrawTransparentDecals", Scene::Cache::Auto);

        mQrDrawTexts = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Enclosure, const Lettering, const Label, ConstPtr<IntColor8>, ConstPtr<Decoration>>
        >("Render::Geometry::DrawTexts", Scene::Cache::Auto);
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

        // Both sprite and decal declare the same feature, so one key selects the cutout variant of either.
        mCutout = mTechniques[Enum::Cast(Kind::Sprite)]->ResolveByName("Cutout");
    }
}
