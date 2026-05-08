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

#include "World.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    World::World(Ref<Service::Host> Host)
        : Locator     { Host },
          mRepository { Host },
          mSupervisor { Host }
    {
        OnRegister(GetService<Scene::Service>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void World::Teardown()
    {
        OnTeardown(GetService<Scene::Service>());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void World::Load()
    {
        mRepository.Load();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void World::Save()
    {
        mRepository.Save();
        mSupervisor.Save();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void World::OnRegister(Ref<Scene::Service> Scene)
    {
        // TODO: Clear separation between components and systems registered by the world.
        Scene.GetComponent<Persist>("Persist");
        Scene.GetComponent<Dispose>("Dispose");
        Scene.GetComponent<Stale>("Stale");
        Scene.GetComponent<Dynamic>("Dynamic");
        Scene.GetComponent<Transform>("Transform");
        Scene.GetComponent<Pose>("Pose").With<Transform>().AddTrait(Scene::Trait::Serializable);
        Scene.GetComponent<Anchor>("Anchor").AddTrait(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Bound>("Bound");
        Scene.GetComponent<Extent>("Extent").AddTrait(Scene::Trait::Serializable, Scene::Trait::Inheritable).With<Bound>();
        Scene.GetComponent<Velocity>("Velocity").With<Dynamic>();
        Scene.GetComponent<Region>("Region").AddTrait(Scene::Trait::Serializable);

        // Observe changes to the pivot component and mark entities as stale if they are not kinetic.
        Scene.CreateObserver<Scene::DSL::With<Anchor>, Scene::DSL::Not<Dynamic>>(
            "World::OnSetOriginMarkDirty",
            EcsOnSet,
            [](Scene::Entity Actor)
            {
                Actor.Add<Stale>();
            });

        // Observe changes to the extent component and mark entities as stale if they are not kinetic.
        Scene.CreateObserver<Scene::DSL::With<Extent>, Scene::DSL::Not<Dynamic>>(
            "World::OnSetDimensionMarkDirty",
            EcsOnSet,
            [](Scene::Entity Actor)
            {
                Actor.Add<Stale>();
            });

        // System that computes motion integration.
        Scene.CreateSystem<Scene::DSL::In<const Time, const Velocity, Pose>>(
            "World::ComputeMotion",
            EcsPreUpdate,
            Scene::Execution::Concurrent,
            [](Scene::Entity Actor, Time Time, ConstRef<Velocity> Velocity, Ref<Pose> Pose)
            {
                Pose.Translate(Velocity.GetLinear() * Time.GetDelta());
                Pose.Rotate(Velocity.GetAngular() * Time.GetDelta());
            });

        // System that migrates entities to a neighbouring region when they cross a region boundary.
        // TODO: Revise (Can't be added efficiently until flecs supports direct parent queries in systems).
        Scene.CreateSystem<Scene::DSL::In<Pose>, Kinetic>(
            "World::RegionMigration",
            EcsPreUpdate,
            Scene::Execution::Concurrent,
            [this](Scene::Entity Actor, Ref<Pose> Pose)
            {
                ConstPtr<Region> Region = nullptr;

                if (const Scene::Entity Parent = Actor.GetParent(); Parent.IsValid())
                {
                    Region = Parent.TryGet<Tileon::Region>();
                }

                if (Region)
                {
                    const Vector2 Position = Pose.GetTranslation();
                    const Vector2 Distance = Vector2(
                        Math::Floor(Position.GetX() / static_cast<Real32>(Tileon::Region::kTilesPerX)),
                        Math::Floor(Position.GetY() / static_cast<Real32>(Tileon::Region::kTilesPerY)));

                    if (Distance.IsAlmostZero())
                    {
                        return;
                    }

                    // Try to migrate to the new region and update the parent-child relationship if successful.
                    const Scene::Entity Parent = mSupervisor.GetRegion(
                        static_cast<SInt16>(Region->GetX() + static_cast<SInt32>(Distance.GetX())),
                        static_cast<SInt16>(Region->GetY() + static_cast<SInt32>(Distance.GetY())));

                    if (Parent.IsValid())
                    {
                        Actor.SetParent(Parent);

                        // Remap the pose to be local to the new region so the entity appears at exactly
                        // the same world position after the parent change.
                        Pose.SetTranslation(Vector2(
                            Position.GetX() - Distance.GetX() * static_cast<Real32>(Tileon::Region::kTilesPerX),
                            Position.GetY() - Distance.GetY() * static_cast<Real32>(Tileon::Region::kTilesPerY)));

                        // Cascades Stale to all static children, making them Kinetic this frame.
                        Actor.Add<Stale>();
                    }
                }
            });

        // System that propagates dirty state to children (for static entities).
        Scene.CreateSystem<Scene::DSL::Not<Stale>, Scene::DSL::Cascade<Stale>, Scene::DSL::Out<Stale>>(
            "World::PropagateDirtyStates",
            EcsPreUpdate,
            Scene::Execution::Concurrent,
            [](Scene::Entity Actor)
            {
                Actor.Add<Stale>();
            });

        // System that computes world matrices from local transforms of kinetic entities.
        Scene.CreateSystem<Scene::DSL::Cascade<ConstPtr<Transform>>, Scene::DSL::In<const Pose, ConstPtr<Anchor>, Transform>, Kinetic>(
            "World::ComputeWorldspace",
            EcsOnUpdate,
            Scene::Execution::Concurrent,
            [](ConstPtr<Transform> Parent, ConstRef<Pose> Pose, ConstPtr<Anchor> Anchor, Ref<Transform> Transform)
            {
                const Vector2 Pivot = Anchor ? Anchor->GetValue() : Vector2::Zero();

                if (Parent)
                {
                    Transform.SetWorldspace(Parent->GetWorldspace() * Pose.Compute(Pivot));
                    Transform.SetOrigin(Parent->GetOrigin());
                }
                else
                {
                    Transform.SetWorldspace(Pose.Compute(Pivot));
                }
            });

        // System that computes world-space volumes from local-space volumes and updates spatial partitioning.
        Scene.CreateSystem<Scene::DSL::In<const Transform, const Extent, Bound>, Kinetic>(
            "World::ComputeHierarchy",
            EcsOnUpdate,
            Scene::Execution::Concurrent,
            [this](Scene::Entity Actor, ConstRef<Transform> Transform, Extent Extent, Ref<Bound> Bounds)
            {
                const Rect LocalAABB(Extent.GetOffset(), Extent.GetOffset() + Extent.GetSize());
                const Rect WorldAABB = Rect::Transform(LocalAABB, Transform.GetWorldspace());

                const IntRect    NewestAABB   = Rect::Enclose<SInt32>(WorldAABB) + Transform.GetOrigin();
                const IntVector2 NewestCenter = NewestAABB.GetCenter();

                if (const IntRect OlderAABB = Bounds.GetRect(); OlderAABB.IsAlmostZero())
                {
                    mSupervisor.InsertEntityOnCell(Actor, NewestCenter);
                }
                else
                {
                    mSupervisor.UpdateEntityOnCell(Actor, OlderAABB.GetCenter(), NewestCenter);
                }
                Bounds.SetRect(NewestAABB);
            });

        // System that optimizes the entity hierarchy (sync point).
        Scene.CreateSystem<>(
            "World::UpdateHierarchy",
            EcsPostUpdate,
            Scene::Execution::Immediate,
            [this]
            {
                mSupervisor.UpdateHierarchy();
            });

        /// System that disposes of entities marked for disposal.
        Scene.CreateSystem<Scene::DSL::In<ConstPtr<Bound>, const Dispose>>(
            "World::DestroyEntitiesTagged",
            EcsPostFrame,
            Scene::Execution::Concurrent,
            [this](Scene::Entity Actor, ConstPtr<Bound> Bounds)
            {
                if (Bounds)
                {
                   mSupervisor.RemoveEntityOnCell(Actor, Bounds->GetRect().GetCenter());
                }
                Actor.Destruct();
            });

        // System for clearing stale tags from entities after processing.
        Scene.CreateSystem(
            "World::ClearStaleTagFromEntities",
            EcsPostFrame,
            Scene::Execution::Immediate,
            [this]
            {
                GetService<Scene::Service>().Purge<Stale>();
            });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void World::OnTeardown(Ref<Scene::Service> Scene)
    {
        mSupervisor.Teardown();
    }
}