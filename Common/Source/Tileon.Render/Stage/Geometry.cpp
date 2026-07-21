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
#include "Tileon.Render/Depth.hpp"
#include "Tileon.World/Component.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Stage
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Geometry::Geometry(Ref<Engine::Subsystem::Host> Host, ConstRef<Tileset> Tileset)
        : Locator     { Host },
          mCanvas     { Host },
          mTileset    { AddressOf(Tileset) },
          mGeneration { 0 }
    {
        OnRegister(* Host.GetService<Scene::Service>());
        OnLoad(* Host.GetService<Content::Service>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Geometry::Run(Ref<Render::Encoder> Encoder)
    {
        ConstRef<Director> Director = (* mDirector);
        ConstRef<Tileset>  Tileset  = (* mTileset);

        const IntRect    Frustum = Director.GetFrustum();
        const IntVector2 Origin(Director.GetPosition().GetBaseX(), Director.GetPosition().GetBaseY());

        // The canvas emits into its collector; the director's view-projection is bound as the frame-global uniform.
        mCanvas.Begin();
        {
            // Draw sprite entities.
            mCanvas.SetTechnique(Render::Canvas::Type::Sprite, mTechniques[Enum::Cast(Kind::SpriteOpaqueWithNormal)]);
            mQrDrawSprites.Run<const Transform, const Extent, const Bound, const Appearance, ConstPtr<IntColor8>>([&](
                ConstRef<Transform>  Transform,
                ConstRef<Extent>     Extent,
                ConstRef<Bound>      Bound,
                ConstRef<Appearance> Appearance,
                ConstPtr<IntColor8>  Tint)
            {
                // Cull against the frustum. A zero bound means the volume was never computed, so keep it.
                if (const IntRect AABB = Bound.GetRect(); !AABB.IsAlmostZero() && !AABB.Test(Frustum))
                {
                    return;
                }

                const IntColor8 Color  = Tint ? (* Tint) : IntColor8::White();
                const Real32    Depth = Depth::Midground(Frustum, Transform.GetOrigin(), Transform.GetWorldspace());

                const Render::Sprite Command(Appearance.GetMaterial(), Extent.GetSize(), Color, Appearance.GetSource());
                mCanvas.DrawSprite(Command, Transform.Rebase(Origin), Depth);
            });

            // Draw text entities.
            mQrDrawTexts.Run<const Transform, const Bound, const Typeface, const Label, ConstPtr<IntColor8>, ConstPtr<Emphasis>>([&](
                ConstRef<Transform> Transform,
                ConstRef<Bound>     Bound,
                ConstRef<Typeface>  Typeface,
                ConstRef<Label>     Label,
                ConstPtr<IntColor8> Tint,
                ConstPtr<Emphasis>  Emphasis)
            {
                // Cull against the frustum. A zero bound means the volume was never computed, so keep it.
                if (const IntRect AABB = Bound.GetRect(); !AABB.IsAlmostZero() && !AABB.Test(Frustum))
                {
                    return;
                }

                const IntColor8 Color = Tint ? (* Tint) : IntColor8::White();
                const Real32    Depth = Depth::Midground(Frustum, Transform.GetOrigin(), Transform.GetWorldspace());

                const Render::TextStyle Stype(Typeface.GetFont(), Typeface.GetSize(), Color, Label.GetSpacing());

                mCanvas.DrawText(Stype, Label.GetContent(), Transform.Rebase(Origin), Depth,
                    Emphasis ? Emphasis->GetEffect() : Render::TextEffect());
            });

            // Draw tile regions, culling against the view frustum to minimize overdraw.
            const UInt32 Generation = Tileset.GetGeneration();
            const Bool   Refreshed  = (mGeneration != Generation);
            mGeneration = Generation;

            mCanvas.SetTechnique(Render::Canvas::Type::Sprite, mTechniques[Enum::Cast(Kind::SpriteOpaque)]);
            mQrDrawRegions.Run<const Region, Mosaic>([&](ConstRef<Region> Region, Ref<Mosaic> Mosaic)
            {
                if (Refreshed)
                {
                    Mosaic.Invalidate();
                }

                const SInt32 WorldRegionX = Region.GetX() * Region::kTilesPerX;
                const SInt32 WorldRegionY = Region.GetY() * Region::kTilesPerY;

                const IntRect Boundaries(
                    WorldRegionX,
                    WorldRegionY,
                    WorldRegionX + Region::kTilesPerX,
                    WorldRegionY + Region::kTilesPerY);

                if (const IntRect Overlap = IntRect::Intersection(Boundaries, Frustum); !Overlap.IsAlmostZero())
                {
                    const IntRect Tiles = Overlap - IntRect(WorldRegionX, WorldRegionY, WorldRegionX, WorldRegionY);
                    DrawRegion(Tileset, Region, Mosaic, Origin, Tiles);
                }
            });
        }
        mCanvas.Flush(Encoder);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Geometry::OnRegister(Ref<Scene::Service> Scene)
    {
        Scene.GetComponent<IntColor8>("Tint").Grant(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Animator>("Animator");
        Scene.GetComponent<Appearance>("Appearance");
        Scene.GetComponent<Animation>("Animation").Grant(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Sprite>("Sprite").Grant(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Label>("Label").Grant(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Typeface>("Typeface").Grant(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Emphasis>("Emphasis").Grant(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Mosaic>("Mosaic");
        Scene.GetComponent<Region>("Region").With<Mosaic>();

        // Attaches a Label to an instance entity that don't have it if it has a typeface.
        Scene.CreateObserver<Scene::DSL::With<Typeface>>(
            "Render::Geometry::ObsAttachLabelWithTypeface",
            EcsOnAdd,
            [](Scene::Entity Actor)
            {
                Actor.Set(Label());
            });

        // Observe tile edits so the cache is rebuilt on the next draw.
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

        // Observe changes to the sprite component to resolve material resources and trigger updates when necessary.
        Scene.CreateObserver<Scene::DSL::In<const Sprite>>(
            "Render::Geometry::ObsUpdateAppearanceOnSpriteUpdate",
            EcsOnSet,
            [this](Scene::Entity Actor, ConstRef<Sprite> Sprite)
            {
                Ref<Content::Service> Content = GetService<Content::Service>();
                Actor.Set(Appearance(Content.Load<Graphic::Material>(Sprite.GetPath()), Sprite.GetSource()));
            }, Scene::DSL::Opt(EcsPrefab));

        // Observe when an animation component is attached, and automatically initialize the animator for playback.
        Scene.CreateObserver<Scene::DSL::In<const Animation>>(
            "Render::Geometry::ObsUpdateAnimatorOnAnimationUpdate",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Animation> Animation)
            {
                Actor.Emplace<Animator>(0.0, Animator::Status::Repeat);
            });

        // Observe changes to the typeface component to resolve font resources and trigger updates when necessary.
        Scene.CreateObserver<Scene::DSL::In<Typeface>>(
            "Render::Geometry::ObsUpdateTypefaceAsync",
            EcsOnSet,
            [this](Scene::Entity Actor, Ref<Typeface> Component)
            {
                // Resolve the font resource and update the component accordingly.
                Ref<Content::Service> Content = GetService<Content::Service>();
                Component.OnResolve(Content);

                if (ConstRetainer<::Render::Font> Font = Component.GetFont(); Font && !Font->HasFinished())
                {
                    Content::Service::Callback Callback = [&Content, Actor](Ref<Content::Resource> Resource)
                    {
                        Content.Unsubscribe(Resource.GetKey());

                        Actor.Notify<Typeface>();   // TODO: Remove double notification
                    };
                    Content.Subscribe(Font->GetKey(), Move(Callback));
                }
            }, Scene::DSL::In(EcsPrefab));

        // Observe changes to the typeface or label components to update the dimension and origin of text entities when necessary.
        Scene.CreateObserver<Scene::DSL::In<const Typeface, const Label>>(
            "Render::Geometry::ObsUpdateTextBoundaries",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Typeface> Typeface, ConstRef<Label> Label)
            {
                if (ConstRetainer<::Render::Font> Font = Typeface.GetFont(); Font && Font->HasFinished())
                {
                    const Pivot2D Pivot   = Label.GetPivot();
                    const Rect    AABB    = Font->Enclose(Label.GetContent(), Typeface.GetSize());
                    const Vector2 Measure = AABB.GetSize();
                    const Vector2 Value(Measure.GetX() * Pivot.GetX(), Measure.GetY() * Pivot.GetY());

                    Actor.Set(Extent { AABB.GetPosition(), AABB.GetSize() });
                    Actor.Set(Anchor { Value });
                }
            });

        // System that advances animations and updates sprite appearances.
        Scene.CreateSystem<Scene::DSL::In<const Scene::Clock, const Animation, Animator, Appearance>>(
            "Render::Geometry::ComputeAnimation",
            EcsOnUpdate,
            Scene::Execution::Concurrent,
            [](ConstRef<Scene::Clock> Clock, ConstRef<Animation> Animation, Ref<Animator> Animator, Ref<Appearance> Appearance)
            {
                // Advance the animator's timestamp and update the current keyframe based on the elapsed time.
                Animator.Advance(Clock.GetAbsolute(), Animation);

                // Update the sprite's appearance with the frame data of the current keyframe.
                Appearance.SetSource(Animation.GetFrameData(Animator.GetKeyframe()));
            });

        // Create the queries for retrieving renderable components.
        mQrDrawSprites = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Extent, const Bound, const Appearance, ConstPtr<IntColor8>>
        >("Render::Geometry::DrawSprites", Scene::Cache::Auto);

        mQrDrawTexts = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Bound, const Typeface, const Label, ConstPtr<IntColor8>, ConstPtr<Emphasis>>
        >("Render::Geometry::DrawTexts", Scene::Cache::Auto);

        mQrDrawRegions = Scene.CreateQuery<
            Scene::DSL::In<const Region, Mosaic>
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

    void Geometry::DrawRegion(ConstRef<Tileset> Tileset, ConstRef<Region> Region, Ref<Mosaic> Mosaic, IntVector2 Origin, IntRect Boundaries)
    {
        const SInt32 RegionX = (Region.GetX() * Region::kTilesPerX) - Origin.GetX();
        const SInt32 RegionY = (Region.GetY() * Region::kTilesPerY) - Origin.GetY();

        // Merging the region's tiles is independent of the camera, so it runs only when the tiles or the
        // tileset glyphs change rather than once per frame.
        if (Mosaic.IsStale())
        {
            Mosaic.Rebuild(Region, Tileset);
        }

        // Iterate through each layer of the region and draw the visible blocks within the specified boundaries.
        for (const Tile::Layer Layer : Enum::GetValues<Tile::Layer>())
        {
            const Real32 Depth = Depth::Background(Enum::Cast(Layer));

            for (ConstRef<Mosaic::Block> Block : Mosaic.GetBlocks(Layer))
            {
                const IntRect Area(Block.X, Block.Y, Block.X + Block.Width, Block.Y + Block.Height);

                // Skip blocks that lie entirely outside the visible slice of the region.
                if (!Area.Test(Boundaries))
                {
                    continue;
                }

                // Fetches the motif corresponding to the terrain handle for the current layer of the block.
                ConstRef<Tileset::Glyph> Glyph = Tileset.GetGlyph(Block.Handle);

                // Draw all merged tiles as a single sprite.
                if (Glyph.Material)
                {
                    const Vector3 Position(RegionX + Block.X, RegionY + Block.Y, Depth);
                    DrawTile(Position, IntVector2(Block.Width, Block.Height), Block.Weight, Glyph);
                }
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Geometry::DrawTile(Vector3 Position, IntVector2 Size, UInt16 Weight, ConstRef<Tileset::Glyph> Glyph)
    {
        const IntVector2 Span = Glyph.Span;
        const Rect       Crop = Glyph.Crop;

        const Real32 UCoordPerTile = Crop.GetWidth()  / Span.GetX();
        const Real32 VCoordPerTile = Crop.GetHeight() / Span.GetY();

        const Real32 OffsetX = (Weight % Span.GetX()) * UCoordPerTile;
        const Real32 OffsetY = (Weight / Span.GetX()) * VCoordPerTile;

        const Rect Displacement(
            Crop.GetX() + OffsetX,
            Crop.GetY() + OffsetY + Size.GetY() * VCoordPerTile,
            Crop.GetX() + OffsetX + Size.GetX() * UCoordPerTile,
            Crop.GetY() + OffsetY);

        const Render::Sprite Command(Glyph.Material, static_cast<Vector2>(Size), Glyph.Tint, Displacement);
        mCanvas.DrawSprite(Command, Matrix3x2::FromTranslation(Position.GetXY()), Position.GetZ());
    }
}