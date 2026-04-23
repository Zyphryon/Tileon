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

#include "Renderer.hpp"
#include "Phase.hpp"
#include "Tileon.World/Component.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Renderer::Renderer(Ref<Service::Host> Host)
        : Locator     { Host },
          mRenderer   { Host },
          mProperties { 0u },
          mTileset    { Host }
    {
        OnLoad(GetService<Content::Service>());
        OnRegister(GetService<Scene::Service>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::Load()
    {
        mTileset.Load();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::Save()
    {
        mTileset.Save();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::SetProperty(Property Mask, Bool Enable)
    {
        // Set or clear the specified property in the renderer's properties bitfield.
        mProperties = SetOrClearBit(mProperties, Enum::Cast(Mask), Enable);

        // Depending on the property being toggled, activate or deactivate the corresponding system in the scene.
        Ref<Scene::Service> Scene = GetService<Scene::Service>();

        switch (Mask)
        {
        case Property::DrawGuide:
            Scene.GetEntity("Renderer::RenderGuide").SetActive(Enable);
            break;
        case Property::DrawVolumes:
            Scene.GetEntity("Renderer::RenderVolumes").SetActive(Enable);
            break;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::Present(ConstRef<Matrix4x4> Projection, IntRect Frustum, IntVector2 Origin)
    {
        mFrustum = Frustum;
        mOrigin  = Origin;

        // Execute the main rendering pass, running all systems in the rendering pipeline.
        mRenderer.Begin(Projection);
        {
            mPipeline.Run(0.0f);
        }
        mRenderer.End();

        // TODO: Other passes like lighting, post-processing, etc.
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::OnLoad(Ref<Content::Service> Content)
    {
        // TODO
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::OnRegister(Ref<Scene::Service> Scene)
    {
        Scene.GetComponent<Tint>("Tint").AddTrait(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Animator>("Animator");
        Scene.GetComponent<Appaerance>("Appaerance");
        Scene.GetComponent<Animation>("Animation").AddTrait(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Sprite>("Sprite").AddTrait(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Text>("Text").AddTrait(Scene::Trait::Serializable);
        Scene.GetComponent<Typeface>("Typeface").AddTrait(Scene::Trait::Serializable, Scene::Trait::Inheritable);

        // Create the rendering phases for the scene, defining the order of execution for rendering systems.
        EcsOnRender = Scene.CreatePhase<"OnRender", Scene::Empty>();

        // Create the main rendering pipeline that will execute systems during the rendering phases.
        mPipeline = Scene.CreatePipeline(Scene::DSL::In(EcsOnRender));

        // Observe changes to the sprite component to resolve material resources and trigger updates when necessary.
        Scene.CreateObserver<Scene::DSL::In<const Sprite>>(
            "Renderer::OnSpriteAttach",
            EcsOnSet,
            [this](Scene::Entity Actor, ConstRef<Sprite> Sprite)
            {
                Actor.Set(Appaerance(GetMaterial(Sprite.GetPath()), Sprite.GetSource()));
            }, Scene::DSL::Not(EcsPrefab));

        // Observe when an Animation component is attached, and automatically initialize the animator for playback.
        Scene.CreateObserver<Scene::DSL::In<const Animation>>(
            "Renderer::OnAnimationAttach",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Animation> Animation)
            {
                Actor.Emplace<Animator>(0.0, Animator::Status::Repeat);
            }, Scene::DSL::Not(EcsPrefab));

        // Observe changes to the typeface component to resolve font resources and trigger updates when necessary.
        Scene.CreateObserver<Scene::DSL::In<Typeface>>(
            "Renderer::OnTypefaceAsyncUpdate",
            EcsOnSet,
            [this](Scene::Entity Actor, Ref<Typeface> Component)
            {
                Ref<Content::Service> Content = GetService<Content::Service>();

                // Resolve the typeface's font resource.
                Component.OnResolve(Content);

                if (ConstTracker<Render::Font> Font = Component.GetFont(); !Font->HasFinished())
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
            "Renderer::OnTypefaceOrTextUpdates",
            EcsOnSet,
            [](Scene::Entity Actor, ConstRef<Typeface> Typeface, ConstRef<Text> Text)
            {
                if (ConstTracker<Render::Font> Font = Typeface.GetFont(); Font->HasFinished())
                {
                    const Pivot Pivot     = Text.GetPivot();
                    const Vector2 Measure = Font->Measure(Text.GetContent(), Typeface.GetSize(), Text.GetSpacing());
                    const Vector2 Anchor(Measure.GetX() * Pivot.GetX(), Measure.GetY() * Pivot.GetY());

                    const Rect AABB = Font->Enclose(Text.GetContent(), Typeface.GetSize(), Text.GetSpacing());
                    Actor.Set(Extent { AABB.GetPosition(), AABB.GetSize() });
                    Actor.Set(Origin { Anchor });
                }
            });

        // System that advances the animations of tilesets.
        Scene.CreateSystem<Scene::DSL::In<const Time>>(
            "Render::ComputeTilesetAnimations",
            EcsOnUpdate,
            Scene::Execution::Immediate,
            [this](Time Time)
            {
                mTileset.Tick(Time.GetAbsolute());
            });

        // System that advances animations and updates sprite appearances.
        Scene.CreateSystem<Scene::DSL::In<const Time, const Animation, Animator, Appaerance>>(
            "Render::ComputeAnimation",
            EcsOnUpdate,
            Scene::Execution::Concurrent,
            [](Time Time, ConstRef<Animation> Animation, Ref<Animator> Animator, Ref<Appaerance> Appaerance)
            {
                // Advance the animator's timestamp and update the current keyframe based on the elapsed time.
                Animator.Advance(Time.GetAbsolute(), Animation);

                // Update the sprite's appearance with the frame data of the current keyframe.
                Appaerance.SetSource(Animation.GetFrameData(Animator.GetKeyframe()));
            });

        // System that renders guide lines for debugging purposes when the corresponding property is enabled.
        Scene.CreateSystem<>(
            "Renderer::RenderGuide",
            EcsOnRender,
            Scene::Execution::Default,
            [this]
            {
                DrawGuide();
            }).Disable();

        // System that renders bounding volumes for debugging purposes when the corresponding property is enabled.
        Scene.CreateSystem<Scene::DSL::In<const Volume>>(
            "Renderer::RenderVolumes",
            EcsOnRender,
            Scene::Execution::Default,
            [this](Volume Volume)
            {
                if (!Volume.IsAlmostZero())
                {
                    mRenderer.SetShift(Vector2::Zero());
                    mRenderer.DrawStrokeRect(Rect(Volume - mOrigin), 0.0f, IntColor8::Green(), 0.1f); // TODO: Depth
                }
            }).Disable();

        // TODO: Frustum Culling System.
        // TODO: Terrain Animation System (we only need to animate once per type not per instance)

        // System that renders sprite entities.
        Scene.CreateSystem<
            Scene::DSL::Up<const Sector>,
            Scene::DSL::In<const Worldspace, const Extent, const Appaerance, ConstPtr<Tint>>>(
            "Renderer::RenderSprite",
            EcsOnRender,
            Scene::Execution::Default,
            [this](Sector Sector, ConstRef<Worldspace> Worldspace, ConstRef<Extent> Extent, ConstRef<Appaerance> Appearance, ConstPtr<Tint> Tint)
            {
                const IntColor8 Color = Tint ? (* Tint) : IntColor8::White();

                const Render::Sprite Command(Appearance.GetMaterial(), Extent.GetSize(), Color, Appearance.GetSource());
                mRenderer.SetShift(static_cast<Vector2>(Sector - mOrigin));
                mRenderer.DrawSprite(Command, Worldspace, 0.0f); // TODO: Depth
            });

        // System that renders text entities.
        Scene.CreateSystem<
            Scene::DSL::Up<const Sector>,
            Scene::DSL::In<const Worldspace, const Typeface, const Text, ConstPtr<Tint>>>(
            "Renderer::RenderText",
            EcsOnRender,
            Scene::Execution::Default,
            [this](Sector Sector, ConstRef<Worldspace> Worldspace, ConstRef<Typeface> Typeface, ConstRef<Text> Text, ConstPtr<Tint> Tint)
            {
                const IntColor8 Color = Tint ? (* Tint) : IntColor8::White();

                const Render::Text Command(Typeface.GetFont(), Typeface.GetSize(), Color, Text.GetSpacing());
                mRenderer.SetShift(static_cast<Vector2>(Sector - mOrigin));
                mRenderer.DrawText(Command, Text.GetContent(), Worldspace, 0.0f, Text.GetEffect()); // TODO: Depth
            });

        // System that renders region entities.
        Scene.CreateSystem<Scene::DSL::In<const Region>>(
            "Renderer::RenderRegion",
            EcsOnRender,
            Scene::Execution::Default,
            [this](ConstRef<Region> Region)
            {
                const SInt32 WorldRegionX = Region.GetX() * Region::kTilesPerX;
                const SInt32 WorldRegionY = Region.GetY() * Region::kTilesPerY;

                const IntRect Boundaries(
                    WorldRegionX,
                    WorldRegionY,
                    WorldRegionX + Region::kTilesPerX,
                    WorldRegionY + Region::kTilesPerY);

                if (const IntRect Overlap = IntRect::Intersection(Boundaries, mFrustum); !Overlap.IsAlmostZero())
                {
                    const IntRect Tiles = Overlap - IntRect(WorldRegionX, WorldRegionY, WorldRegionX, WorldRegionY);
                    DrawRegion(Region, Tiles);
                }
            });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::DrawRegion(ConstRef<Region> Region, IntRect Boundaries)
    {
        const SInt32 RegionX = Region.GetX() * Region::kTilesPerX;
        const SInt32 RegionY = Region.GetY() * Region::kTilesPerY;

        // Calculate bitmasks for efficiently determining which tiles to render based on the visible boundaries.
        const UInt32 TileYMask = ((1ull << (Boundaries.GetWidth())) - 1ull) << Boundaries.GetX();

        // Iterate through each layer of the region and draw the visible tiles within the specified boundaries.
        for (const Tile::Layer Layer : Enum::Values<Tile::Layer>())
        {
            Array<UInt32, Region::kTilesPerX> Resolution { };

            const Real32 Depth = 1.0f - Enum::Cast(Layer) * 0.01f; // TODO: Depth

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
                    const UInt8  Weight = Tile.GetWeight(Layer);

                    if (Handle == 0)
                    {
                        continue;
                    }

                    // Fetches the tileset entry corresponding to the terrain handle for the current layer of the tile.
                    Ref<Tileset::Entry> Entry = mTileset.GetEntry(Handle);

                    if (Entry.IsValid())
                    {
                        const UInt32 MaxInnerX = Min(Boundaries.GetMaximumX(), TileX + (Entry.Columns - Weight % Entry.Columns));
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

                            const UInt8 ExpectedWeight = Weight + (InnerX - TileX);

                            ConstRef<Tileon::Tile> InnerTile = Region.GetTile(InnerX, TileY);

                            if (InnerTile.GetHandle(Layer) != Handle || InnerTile.GetWeight(Layer) != ExpectedWeight)
                            {
                                break;
                            }
                        }

                        const UInt32 Mask = ((1u << CountX) - 1) << TileX;
                        Resolution[TileY] |= Mask;

                        // Try to expand vertically to adjacent rows that share the same terrain.
                        UInt8 RowWeight = Weight + Entry.Columns;

                        for (UInt32 InnerY = TileY + 1; InnerY < MaxInnerY; ++InnerY, ++CountY, RowWeight += Entry.Columns)
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

                                const UInt8 ExpectedWeight = RowWeight + (InnerX - TileX);

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

                        // TODO: Remove this from here and cache it at start.
                        if (Entry.Material == nullptr)
                        {
                            Entry.Material = GetMaterial(Entry.Path);
                        }

                        // Draw all merged tiles as a single sprite.
                        const Vector3 Position(RegionX + TileX, RegionY + TileY, Depth);
                        DrawTile(Position, IntVector2(CountX, CountY), Weight, Entry);

                        // Advance the X coordinate by the number of merged tiles to skip over them in the iteration.
                        TileX += CountX - 1;
                    }
                    else
                    {
                        // TODO: Draw Default Material?
                    }
                }
            }
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::DrawTile(Vector3 Position, IntVector2 Span, UInt8 Weight, ConstRef<Tileset::Entry> Visual)
    {
        const Rect Frame = Visual.Animation.GetFrameData(Visual.Keyframe);

        const Real32 UCoordPerTile = Frame.GetWidth()  / Visual.Columns;
        const Real32 VCoordPerTile = Frame.GetHeight() / Visual.Rows;

        const Real32 OffsetX = (Weight % Visual.Columns) * UCoordPerTile;
        const Real32 OffsetY = (Weight / Visual.Rows) * VCoordPerTile;

        const Rect Displacement(
            Frame.GetX() + OffsetX,
            Frame.GetY() + OffsetY + Span.GetY() * VCoordPerTile,
            Frame.GetX() + OffsetX + Span.GetX() * UCoordPerTile,
            Frame.GetY() + OffsetY);

        const Render::Sprite Command(Visual.Material, static_cast<Vector2>(Span), Tint::White(), Displacement);
        mRenderer.SetShift(Vector2(Position.GetX() - mOrigin.GetX(), Position.GetY() - mOrigin.GetY()));
        mRenderer.DrawSprite(Command, Matrix3x2::Identity(), Position.GetZ());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Renderer::DrawGuide()
    {
        // TODO: Redo in shader (supporting dynamic zoom levels and colors)
        constexpr IntColor8 Tint = IntColor8::Gray();

        const Real32 MinY = mFrustum.GetMinimumY();
        const Real32 MaxY = mFrustum.GetMaximumY();
        const Real32 MinX = mFrustum.GetMinimumX();
        const Real32 MaxX = mFrustum.GetMaximumX();

        mRenderer.SetShift(Vector2(-mOrigin));

        for (SInt32 X = mFrustum.GetMinimumX(); X < mFrustum.GetMaximumX(); ++X)
        {
            const Real32 Thickness = (X % Region::kTilesPerX == 0) ? 0.1f : 0.05f;
            mRenderer.DrawLine(Line(Vector2(X, MinY), Vector2(X, MaxY)), 0.0f, Tint, Thickness); // TODO: Depth
        }

        for (SInt32 Y = mFrustum.GetMinimumY(); Y < mFrustum.GetMaximumY(); ++Y)
        {
            const Real32 Thickness = (Y % Region::kTilesPerY == 0) ? 0.1f : 0.05f;
            mRenderer.DrawLine(Line(Vector2(MinX, Y), Vector2(MaxX, Y)), 0.0f, Tint, Thickness); // TODO: Depth
        }
    }
}

