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

    Geometry::Geometry(Ref<Service::Host> Host)
        : Locator { Host },
          mCanvas { Host }
    {
        OnRegister(* Host.GetService<Scene::Service>());
        OnLoad(* Host.GetService<Content::Service>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Geometry::Run(ConstRef<Director> Director, ConstRef<Tileset> Tileset)
    {
        const IntRect    Frustum = Director.GetFrustum();
        const IntVector2 Origin(Director.GetPosition().GetBaseX(), Director.GetPosition().GetBaseY());

        mCanvas.Begin(Director.GetViewProjection());
        {
            // Draw sprite entities.
            // TODO: Frustum Culling
            mCanvas.SetPipeline(mPipelines[Enum::Cast(Technique::SpriteOpaqueWithNormal)]);
            mQrDrawSprites.Run<const Transform, const Extent, const Appaerance, ConstPtr<IntColor8>>([&](
                ConstRef<Transform>  Transform,
                ConstRef<Extent>     Extent,
                ConstRef<Appaerance> Appearance,
                ConstPtr<IntColor8>  Tint)
            {
                const IntColor8 Color  = Tint ? (* Tint) : IntColor8::White();
                const Real32    Depth = Depth::Midground(Frustum, Transform.GetOrigin(), Transform.GetWorldspace());

                const Render::Sprite Command(Appearance.GetMaterial(), Extent.GetSize(), Color, Appearance.GetSource());
                mCanvas.DrawSprite(Command, Transform.Rebase(Origin), Depth);
            });

            // Draw text entities.
            // TODO: Frustum Culling
            mQrDrawTexts.Run<const Transform, const Typeface, const Text, ConstPtr<IntColor8>>([&](
                ConstRef<Transform> Transform,
                ConstRef<Typeface>  Typeface,
                ConstRef<Text>      Text,
                ConstPtr<IntColor8> Tint)
            {
                const IntColor8 Color = Tint ? (* Tint) : IntColor8::White();
                const Real32    Depth = Depth::Midground(Frustum, Transform.GetOrigin(), Transform.GetWorldspace());

                const Render::Text Command(Typeface.GetFont(), Typeface.GetSize(), Color, Text.GetSpacing());
                mCanvas.DrawText(Command, Text.GetContent(), Transform.Rebase(Origin), Depth, Text.GetEffect());
            });

            // Draw tile regions, culling against the view frustum to minimize overdraw.
            // TODO: SpriteOpaqueWithNormal?
            mCanvas.SetPipeline(mPipelines[Enum::Cast(Technique::SpriteOpaque)]);
            mQrDrawRegions.Run<const Region>([&](ConstRef<Region> Region)
            {
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
                    DrawRegion(Tileset, Region, Origin, Tiles);
                }
            });
        }
        mCanvas.End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Geometry::OnRegister(Ref<Scene::Service> Scene)
    {
        Scene.GetComponent<IntColor8>("Tint").Grant(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Animator>("Animator");
        Scene.GetComponent<Appaerance>("Appaerance");
        Scene.GetComponent<Animation>("Animation").Grant(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Sprite>("Sprite").Grant(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Text>("Text").Grant(Scene::Trait::Serializable);
        Scene.GetComponent<Typeface>("Typeface").Grant(Scene::Trait::Serializable, Scene::Trait::Inheritable);

        // Observe changes to the sprite component to resolve material resources and trigger updates when necessary.
        Scene.CreateObserver<Scene::DSL::In<const Sprite>>(
            "Render::Geometry::ObsUpdateAppaeranceOnSpriteUpdate",
            EcsOnSet,
            [this](Scene::Entity Actor, ConstRef<Sprite> Sprite)
            {
                Ref<Content::Service> Content = GetService<Content::Service>();
                Actor.Set(Appaerance(Content.Load<Graphic::Material>(Sprite.GetPath()), Sprite.GetSource()));
            }, Scene::DSL::Not(EcsPrefab));

        // Observe when an Animation component is attached, and automatically initialize the animator for playback.
        Scene.CreateObserver<Scene::DSL::In<const Animation>>(
            "Render::Geometry::ObsUpdateAnimatorOnAnimationUpdate",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Animation> Animation)
            {
                Actor.Emplace<Animator>(0.0, Animator::Status::Repeat);
            }, Scene::DSL::Not(EcsPrefab));

        // Observe changes to the typeface component to resolve font resources and trigger updates when necessary.
        Scene.CreateObserver<Scene::DSL::In<Typeface>>(
            "Render::Geometry::ObsUpdateTypefaceAsync",
            EcsOnSet,
            [this](Scene::Entity Actor, Ref<Typeface> Component)
            {
                // Resolve the font resource and update the component accordingly.
                Ref<Content::Service> Content = GetService<Content::Service>();
                Component.OnResolve(Content);

                if (ConstTracker<::Render::Font> Font = Component.GetFont(); !Font->HasFinished())
                {
                    Content::Service::AssetDelegate Callback = [&Content, Actor](Ref<Content::Resource> Resource)
                    {
                        Content.Unsubscribe(Resource.GetKey());

                        Actor.Notify<Typeface>();   // TODO: Remove double notification
                    };
                    Content.Subscribe(Font->GetKey(), Move(Callback));
                }
            }, Scene::DSL::In(EcsPrefab));

        // Observe changes to the typeface or text components to update the dimension and origin of text entities when necessary.
        Scene.CreateObserver<Scene::DSL::In<const Typeface, const Text>>(
            "Render::Geometry::ObsUpdateTextBoundaries",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Typeface> Typeface, ConstRef<Text> Text)
            {
                if (ConstTracker<::Render::Font> Font = Typeface.GetFont(); Font && Font->HasFinished())
                {
                    const Pivot   Pivot   = Text.GetPivot();
                    const Vector2 Measure = Font->Measure(Text.GetContent(), Typeface.GetSize(), Text.GetSpacing());
                    const Vector2 Value(Measure.GetX() * Pivot.GetX(), Measure.GetY() * Pivot.GetY());

                    const Rect AABB = Font->Enclose(Text.GetContent(), Typeface.GetSize(), Text.GetSpacing());
                    Actor.Set(Extent { AABB.GetPosition(), AABB.GetSize() });
                    Actor.Set(Anchor { Value });
                }
            });

        // System that advances animations and updates sprite appearances.
        Scene.CreateSystem<Scene::DSL::In<const Time, const Animation, Animator, Appaerance>>(
            "Render::Geometry::ComputeAnimation",
            EcsOnUpdate,
            Scene::Execution::Concurrent,
            [](Time Time, ConstRef<Animation> Animation, Ref<Animator> Animator, Ref<Appaerance> Appaerance)
            {
                // Advance the animator's timestamp and update the current keyframe based on the elapsed time.
                Animator.Advance(Time.GetAbsolute(), Animation);

                // Update the sprite's appearance with the frame data of the current keyframe.
                Appaerance.SetSource(Animation.GetFrameData(Animator.GetKeyframe()));
            });

        // Create the queries for retrieving renderable components.
        mQrDrawSprites = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Extent, const Appaerance, ConstPtr<IntColor8>>
        >("Render::Geometry::DrawSprites", Scene::Cache::Auto);

        mQrDrawTexts = Scene.CreateQuery<
            Scene::DSL::In<const Transform, const Typeface, const Text, ConstPtr<IntColor8>>
        >("Render::Geometry::DrawTexts", Scene::Cache::Auto);

        mQrDrawRegions = Scene.CreateQuery<
            Scene::DSL::In<const Region>
        >("Render::Geometry::DrawRegions", Scene::Cache::Auto);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Geometry::OnLoad(Ref<Content::Service> Content)
    {
        for (const Technique Type : Enum::Values<Technique>())
        {
            const ConstStr8 Path = Format("Resources://Pipeline/Geometry/{}.effect", Enum::Name(Type));

            mPipelines[Enum::Cast(Type)] = Content.Load<Graphic::Pipeline>(Path);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Geometry::DrawRegion(ConstRef<Tileset> Tileset, ConstRef<Region> Region, IntVector2 Origin, IntRect Boundaries)
    {
        const SInt32 RegionX = (Region.GetX() * Region::kTilesPerX) - Origin.GetX();
        const SInt32 RegionY = (Region.GetY() * Region::kTilesPerY) - Origin.GetY();

        // Calculate bitmasks for efficiently determining which tiles to render based on the visible boundaries.
        const UInt32 TileYMask = ((1ull << (Boundaries.GetWidth())) - 1ull) << Boundaries.GetX();

        // Iterate through each layer of the region and draw the visible tiles within the specified boundaries.
        for (const Tile::Layer Layer : Enum::Values<Tile::Layer>())
        {
            Array<UInt32, Region::kTilesPerX> Resolution { };

            const Real32 Depth = Depth::Background(Enum::Cast(Layer));

            for (UInt32 TileY = Boundaries.GetMinimumY(); TileY < Boundaries.GetMaximumY(); ++TileY)
            {
                // Skip rows that have no visible tiles based on the precomputed bitmask for the current Y coordinate.
                if (HasBit(Resolution[TileY], TileYMask))
                {
                    continue;
                }

                for (UInt32 TileX = Boundaries.GetMinimumX(); TileX < Boundaries.GetMaximumX(); ++TileX)
                {
                    // Skip tiles that are empty based on the precomputed bitmask for the current X coordinate within the row.
                    if (HasBit(Resolution[TileY], 1u << TileX))
                    {
                        continue;
                    }

                    // Fetches the tile at the specified coordinates within the region.
                    ConstRef<Tile> Tile = Region.GetTile(TileX, TileY);
                    const UInt16 Handle = Tile.GetHandle(Layer);
                    const UInt16 Weight = Tile.GetWeight(Layer);

                    if (Handle == 0)
                    {
                        continue;
                    }

                    // Fetches the motif corresponding to the terrain handle for the current layer of the tile.
                    ConstRef<Tileset::Glyph> Glyph = Tileset.GetGlyph(Handle);

                    const IntVector2 Span  = Glyph.Span;
                    const UInt32 MaxInnerX = Min(Boundaries.GetMaximumX(), TileX + (Span.GetX() - Weight % Span.GetX()));
                    const UInt32 MaxInnerY = Boundaries.GetMaximumY();
                    UInt8        CountX    = 1;
                    UInt8        CountY    = 1;

                   // Try to expand horizontally to adjacent columns that share the same terrain.
                   for (UInt32 InnerX = TileX + 1; InnerX < MaxInnerX; ++InnerX, ++CountX)
                   {
                       if (HasBit(Resolution[TileY], 1u << InnerX))
                       {
                           break;
                       }

                       const UInt16 ExpectedWeight = Weight + (InnerX - TileX);

                       ConstRef<Tileon::Tile> InnerTile = Region.GetTile(InnerX, TileY);

                       if (InnerTile.GetHandle(Layer) != Handle || InnerTile.GetWeight(Layer) != ExpectedWeight)
                       {
                           break;
                       }
                   }

                   const UInt32 Mask = ((1u << CountX) - 1) << TileX;
                   Resolution[TileY] |= Mask;

                   // Try to expand vertically to adjacent rows that share the same terrain.
                   UInt16 RowWeight = Weight + Span.GetX();

                   for (UInt32 InnerY = TileY + 1; InnerY < MaxInnerY; ++InnerY, ++CountY, RowWeight += Span.GetX())
                   {
                       Bool Merge = true;

                       for (UInt32 InnerX = TileX; InnerX < TileX + CountX; ++InnerX)
                       {
                           if (HasBit(Resolution[InnerY], 1u << InnerX))
                           {
                               Merge = false;
                               break;
                           }

                           ConstRef<Tileon::Tile> InnerTile = Region.GetTile(InnerX, InnerY);

                           const UInt16 ExpectedWeight = RowWeight + (InnerX - TileX);

                           if (InnerTile.GetHandle(Layer) != Handle || InnerTile.GetWeight(Layer) != ExpectedWeight)
                           {
                               Merge = false;
                               break;
                           }
                       }

                       if (Merge)
                       {
                           Resolution[InnerY] |= Mask;
                       }
                       else
                       {
                           break;
                       }
                   }

                   // Draw all merged tiles as a single sprite.
                   if (Glyph.Material)
                   {
                       const Vector3 Position(
                           RegionX + static_cast<SInt32>(TileX),
                           RegionY + static_cast<SInt32>(TileY), Depth);
                       DrawTile(Position, IntVector2(CountX, CountY), Weight, Glyph);
                   }

                   // Advance the X coordinate by the number of merged tiles to skip over them in the iteration.
                   TileX += CountX - 1;
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

        // TODO: Cache in Glyph to prevent two division per draw?
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