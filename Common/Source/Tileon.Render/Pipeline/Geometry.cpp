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

namespace Tileon::Pipeline
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Vector3 Measure(Ref<Appearance> Appearance, UInt16 Density)
    {
        const Rect    Source = Appearance.GetSource();
        const Vector2 Sheet  = Appearance.GetSheet();

        return Vector3(Source.GetWidth() * Sheet.GetX() / Density, Source.GetHeight() * Sheet.GetY() / Density, 0.0f);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Geometry::Geometry(Ref<Engine::Subsystem::Host> Host, ConstRef<Tileset> Tileset)
        : Locator   { Host },
          mTileset  { Tileset },
          mDirector { nullptr },
          mDensity  { 0 },
          mTiles    { Host.GetService<Graphic::Service>() },
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
        ZY_PROFILE_SCOPE("Pipeline::Geometry::Run");

        const IntVector3 Origin  = IntVector3::FromXZ(
            IntVector2(mDirector->GetPosition().GetBaseX(),
                       mDirector->GetPosition().GetBaseY()));

        // Opaque entities go down first, so the ground behind them is rejected by depth rather than shaded twice.
        mCollector.Begin(Render::Collector::Priority::Opaque);

        {
            ZY_PROFILE_SCOPE("Pipeline::Geometry::Opaque");

            mSprites.Reset();

            // Draw opaque unlit sprite entities.
            mSprites.SetTechnique(mTechniques[Enum::Cast(Kind::Sprite_Opaque)]);
            mQrDrawOpaqueUnlitSprites.Run<>([&](
                ConstRef<Transform>  Transform,
                ConstRef<Extent>     Extent,
                ConstRef<Enclosure>  Enclosure,
                ConstRef<Appearance> Appearance,
                ConstPtr<IntColor8>  Tint)
            {
                if (ConstRef<IntBox> AABB = Enclosure.GetVolume(); AABB.IsAlmostZero() || mDirector->IsVisible(AABB))
                {
                    const Matrix4x3 Matrix = Transform.Rebase(Origin);

                    mSprites.Draw(Appearance, Extent.GetSize().GetXY(), Matrix, Tint ? (* Tint) : IntColor8::White());
                }
            });

            // Draw opaque lit sprite entities.
            mSprites.SetTechnique(mTechniques[Enum::Cast(Kind::Sprite_Opaque_Lit)]);
            mQrDrawOpaqueLitSprites.Run<>([&](
                ConstRef<Transform>  Transform,
                ConstRef<Extent>     Extent,
                ConstRef<Enclosure>  Enclosure,
                ConstRef<Appearance> Appearance,
                ConstPtr<IntColor8>  Tint)
            {
                if (ConstRef<IntBox> AABB = Enclosure.GetVolume(); AABB.IsAlmostZero() || mDirector->IsVisible(AABB))
                {
                    const Matrix4x3 Matrix = Transform.Rebase(Origin);

                    mSprites.Draw(Appearance, Extent.GetSize().GetXY(), Matrix, Tint ? (* Tint) : IntColor8::White());
                }
            });
        }
        Drain(Encoder);

        // The ground follows, drawn front to back so the layers above reject the base layer they cover.
        mTiles.Reset();
        {
            ZY_PROFILE_SCOPE("Pipeline::Geometry::Tiles");

            constexpr auto kLayers = Enum::GetValues<Tile::Layer>();

            for (UInt32 Index = kLayers.GetSize(); Index > 0; --Index)
            {
                const Tile::Layer Layer = kLayers[Index - 1];

                // Only the base layer is guaranteed to cover the ground whole, so only it can skip the alpha test.
                mTiles.SetTechnique(Layer == Tile::Layer::Base
                    ? mTechniques[Enum::Cast(Kind::Tile_Opaque)]
                    : mTechniques[Enum::Cast(Kind::Tile_Masked)]);

                // Draw region entities.
                mQrDrawRegions.Run<>([&](ConstRef<Region> Region, Ref<Mosaic> Mosaic)
                {
                    const SInt32 WorldRegionX = Region.GetX() * Region::kTilesPerX;
                    const SInt32 WorldRegionY = Region.GetY() * Region::kTilesPerY;

                    const IntRect Frustum = mDirector->GetFrustum();
                    const IntRect Boundaries(
                        WorldRegionX,
                        WorldRegionY,
                        WorldRegionX + Region::kTilesPerX,
                        WorldRegionY + Region::kTilesPerY);

                    if (const IntRect Overlap = IntRect::Intersection(Boundaries, Frustum); !Overlap.IsAlmostZero())
                    {
                        const IntRect Tiles = Overlap - IntRect(WorldRegionX, WorldRegionY, WorldRegionX, WorldRegionY);
                        DrawRegion(Region, Mosaic, Origin, Tiles, Layer);
                    }
                });
            }
        }
        mTiles.Flush(Encoder);

        // Everything that blends comes last, drained back to front by the collector.
        mCollector.Begin(Render::Collector::Priority::Transparent);
        {
            ZY_PROFILE_SCOPE("Pipeline::Geometry::Transparent");

            mSprites.Reset();

            // Draw transparent unlit sprite entities.
            mSprites.SetTechnique(mTechniques[Enum::Cast(Kind::Sprite_Transparent)]);
            mQrDrawTransparentUnlitSprites.Run<>([&](
                ConstRef<Transform>  Transform,
                ConstRef<Extent>     Extent,
                ConstRef<Enclosure>  Enclosure,
                ConstRef<Appearance> Appearance,
                ConstPtr<IntColor8>  Tint)
            {
                if (ConstRef<IntBox> AABB = Enclosure.GetVolume(); AABB.IsAlmostZero() || mDirector->IsVisible(AABB))
                {
                    const Matrix4x3 Matrix = Transform.Rebase(Origin);

                    mSprites.Draw(Appearance, Extent.GetSize().GetXY(), Matrix, Tint ? (* Tint) : IntColor8::White());
                }
            });

            // Draw transparent lit sprite entities.
            mSprites.SetTechnique(mTechniques[Enum::Cast(Kind::Sprite_Transparent_Lit)]);
            mQrDrawTransparentLitSprites.Run<>([&](
                ConstRef<Transform>  Transform,
                ConstRef<Extent>     Extent,
                ConstRef<Enclosure>  Enclosure,
                ConstRef<Appearance> Appearance,
                ConstPtr<IntColor8>  Tint)
            {
                if (ConstRef<IntBox> AABB = Enclosure.GetVolume(); AABB.IsAlmostZero() || mDirector->IsVisible(AABB))
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
                if (ConstRef<IntBox> AABB = Enclosure.GetVolume(); AABB.IsAlmostZero() || mDirector->IsVisible(AABB))
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

    void Geometry::Drain(Ref<Render::Encoder> Encoder)
    {
        ZY_PROFILE_SCOPE("Pipeline::Geometry::Drain");

        // The runs the glyph batches index into are uploaded here, so a drain can never read a stale palette.
        mGlyphs.Prepare();

        // Sprites and text share the queue, so one sorted drain hands each batch back to whichever recorded it.
        mCollector.Poll([&](UInt32 Kind, ConstSpan<Render::Collector::Command> Commands)
        {
            switch (static_cast<Batch>(Kind))
            {
            case Batch::Sprite:
                mSprites.Write(Encoder, Commands);
                break;
            case Batch::Glyph:
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
            Scene::DSL::Declare<Animator, Appearance, Mosaic>(),
            Scene::DSL::Declare<IntColor8>("Tint", Scene::DSL::Authored),
            Scene::DSL::Declare<Transparent, Unlit>(Scene::DSL::Authored),
            Scene::DSL::Declare<Animation, Decoration, Label, Lettering, Sprite>(Scene::DSL::Authored));

        // Observe when a region is attached, and automatically give it the mosaic its tiles are drawn from.
        Scene.CreateObserver<Scene::DSL::With<Region>>(
            "Render::Geometry::ObsAttachMosaic",
            EcsOnAdd,
            [](Scene::Entity Actor)
            {
                Actor.Emplace<Mosaic>();
            });

        // Observe changes to the region component to invalidate its mosaic, so the merged runs of tiles the
        // stage draws are rebuilt from the tiles the region now holds.
        Scene.CreateObserver<Scene::DSL::With<Region>>(
            "Render::Geometry::ObsInvalidateMosaicOnRegionUpdate",
            EcsOnSet,
            [](Scene::Entity Actor)
            {
                if (const Ptr<Mosaic> Cache = Actor.TryGet<Mosaic>())
                {
                    Cache->Invalidate();
                }
            });

        // Observe when a lettering component is attached, and automatically provide the label it sets the text of.
        Scene.CreateObserver<Scene::DSL::With<Lettering>>(
            "Render::Geometry::ObsAttachLabelWithLettering",
            EcsOnAdd,
            [](Scene::Entity Actor)
            {
                Actor.Emplace<Label>();
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
                        Vector2 Sheet = Vector2::Zero();

                        if (Material->HasCompleted())
                        {
                            if (ConstRetainer<Graphic::Image> Albedo = Material->GetImage("Albedo"_Hash))
                            {
                                Sheet = Vector2(Albedo->GetWidth(), Albedo->GetHeight());
                            }
                        }

                        Appearance Appearance(Material, Component.GetSource(), Sheet);

                        Actor.Set(Extent(Vector3::Zero(), Measure(Appearance, mDensity)));
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
            [this](ConstRef<Scene::Clock> Clock,
                   ConstRef<Animation>    Animation,
                   Ref<Animator>          Animator,
                   Ref<Appearance>        Appearance,
                   Ref<Extent>            Extent)
            {
                // Advance the animator's timestamp and update the current keyframe based on the elapsed time.
                Animator.Advance(Clock.GetAbsolute(), Animation);

                // A keyframe cropping the same rectangle as the one before it leaves the quad alone, so a
                // sprite whose frames share a size measures once and never again.
                if (const Rect Source = Animation.GetFrameData(Animator.GetKeyframe()); Source != Appearance.GetSource())
                {
                    Appearance.SetSource(Source);

                    Extent.SetSize(Measure(Appearance, mDensity));
                }
            });

        mQrDrawOpaqueUnlitSprites = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Extent, const Enclosure, const Appearance, ConstPtr<IntColor8>>,
            Scene::DSL::With<Unlit>,
            Scene::DSL::Not<Transparent>
        >("Render::Geometry::DrawOpaqueUnlitSprites", Scene::Cache::Auto);

        mQrDrawOpaqueLitSprites = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Extent, const Enclosure, const Appearance, ConstPtr<IntColor8>>,
            Scene::DSL::Not<Transparent, Unlit>
        >("Render::Geometry::DrawOpaqueLitSprites", Scene::Cache::Auto);

        mQrDrawTransparentUnlitSprites = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Extent, const Enclosure, const Appearance, ConstPtr<IntColor8>>,
            Scene::DSL::With<Transparent, Unlit>
        >("Render::Geometry::DrawTransparentUnlitSprites", Scene::Cache::Auto);

        mQrDrawTransparentLitSprites = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Extent, const Enclosure, const Appearance, ConstPtr<IntColor8>>,
            Scene::DSL::With<Transparent>,
            Scene::DSL::Not<Unlit>
        >("Render::Geometry::DrawTransparentLitSprites", Scene::Cache::Auto);

        mQrDrawTexts = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Enclosure, const Lettering, const Label, ConstPtr<IntColor8>, ConstPtr<Decoration>>
        >("Render::Geometry::DrawTexts", Scene::Cache::Auto);

        mQrDrawRegions = Scene.CreateQuery<
            Scene::DSL::In<Region>, Scene::DSL::InOut<Mosaic>
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

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Geometry::DrawRegion(ConstRef<Region> Region, Ref<Mosaic> Mosaic, IntVector3 Origin, IntRect Boundaries, Tile::Layer Layer)
    {
        const IntVector2 Ground  = Origin.GetXZ();
        const SInt32     RegionX = (Region.GetX() * Region::kTilesPerX) - Ground.GetX();
        const SInt32     RegionY = (Region.GetY() * Region::kTilesPerY) - Ground.GetY();

        if (Mosaic.IsInvalidated())
        {
            Mosaic.Rebuild(Region);
        }

        // Draw the visible blocks of the layer within the specified boundaries.
        for (ConstRef<Mosaic::Block> Block : Mosaic.GetBlocks(Layer))
        {
            const IntRect Area(Block.X, Block.Y, Block.X + Block.Width, Block.Y + Block.Height);

            // Skip blocks that lie entirely outside the visible slice of the region.
            if (!Area.Test(Boundaries))
            {
                continue;
            }

            // Draw all merged tiles as a single tile instance, once its image has reached the atlas.
            if (ConstRef<Tileset::Glyph> Glyph = mTileset.GetGlyph(Block.Handle); Glyph.Texture)
            {
                const IntVector2 Position(RegionX + Block.X, RegionY + Block.Y);
                const IntVector2 Span(Block.Width, Block.Height);
                const IntVector2 Absolute(
                    Region.GetX() * Region::kTilesPerX + Block.X,
                    Region.GetY() * Region::kTilesPerY + Block.Y);

                const IntVector2 Phase = Mosaic::GetPhase(Absolute, Glyph.Period, Block.Offset);
                mTiles.Draw(Glyph, Phase, Position, Span, Enum::Cast(Layer));
            }
        }
    }
}