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

    World::World(Ref<Engine::Subsystem::Host> Host)
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
        Scene.Register(Scene::DSL::Declare<Persist, Dispose, Stale, Dynamic, Unpickable>());
        Scene.Register(Scene::DSL::Declare<Enclosure, Transform>());
        Scene.Register(Scene::DSL::Declare<Region>(Scene::DSL::Serializable));
        Scene.Register(Scene::DSL::Declare<Anchor>(Scene::DSL::Authored));
        Scene.Register(Scene::DSL::Declare<Extent>(Scene::DSL::Implies<Enclosure>));
        Scene.Register(Scene::DSL::Declare<Pose>(Scene::DSL::Authored, Scene::DSL::Implies<Transform, Extent>));
        Scene.Register(Scene::DSL::Declare<Velocity>(Scene::DSL::Authored, Scene::DSL::Implies<Dynamic>));

        // Observe changes to the local transform and mark entities as stale if they are not kinetic.
        Scene.CreateObserver<Scene::DSL::With<Pose>, Scene::DSL::Not<Dynamic>>(
            "World::OnSetPoseMarkDirty",
            EcsOnSet,
            [](Scene::Entity Actor)
            {
                Actor.Add<Stale>();
            });

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
        Scene.CreateSystem<Scene::DSL::Write<Pose, Stale>>(
            "World::ComputeMotion",
            EcsPreUpdate,
            Scene::Execution::Default,
            [](Scene::Entity Actor, ConstRef<Scene::Clock> Clock, ConstRef<Velocity> Velocity, Ref<Pose> Pose)
            {
                const Vector3 Linear  = Velocity.GetLinear();
                const Vector3 Angular = Velocity.GetAngular();

                if (Linear.IsAlmostZero() && Angular.IsAlmostZero())
                {
                    return;
                }

                const Real32  Delta = static_cast<Real32>(Clock.GetDelta());

                Pose.Translate(Linear * Delta);

                if (!Angular.IsAlmostZero())
                {
                    Pose.Rotate(Velocity.Integrate(Delta));
                }
                Actor.Add<Stale>();
            });

        // System that migrates an entity to a neighbouring region once it crosses a region boundary.
        Scene.CreateSystem<Kinetic, Scene::DSL::Read<Region>, Scene::DSL::Write<Stale>>(
            "World::RegionMigration",
            EcsPreUpdate,
            Scene::Execution::Default,
            [this](Scene::Entity Actor, Ref<Pose> Pose)
            {
                mSupervisor.Migrate(Actor, Pose);
            }, Scene::DSL::Not(ecs_id(EcsParent)));

        // System that propagates stale states to all descendants of a stale entity.
        Scene.CreateSystem<Scene::DSL::Not<Stale>, Scene::DSL::Up<Stale>, Scene::DSL::Write<Stale>>(
            "World::PropagateDirtyStates",
            EcsPreUpdate,
            Scene::Execution::Default,
            [](Scene::Entity Actor)
            {
                Actor.Add<Stale>();
            });

        // System that computes world matrices from local transforms of kinetic entities.
        Scene.CreateSystem<Scene::DSL::Cascade<ConstPtr<Transform>>,
                           Scene::DSL::In<Pose, Ptr<Anchor>>,
                           Scene::DSL::InOut<Transform>,
                           Kinetic>(
            "World::ComputeWorldspace",
            EcsOnUpdate,
            Scene::Execution::Default,
            [](ConstPtr<Transform> Parent, ConstRef<Pose> Pose, ConstPtr<Anchor> Anchor, Ref<Transform> Transform)
            {
                const Vector3 Pivot = Anchor ? Anchor->GetValue() : Vector3::Zero();

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
        Scene.CreateSystem<Kinetic>(
            "World::ComputeHierarchy",
            EcsOnUpdate,
            Scene::Execution::Default,
            [this](Scene::Entity Actor, ConstRef<Transform> Transform, ConstRef<Extent> Extent, Ref<Enclosure> Enclosure)
            {
                const Box        WorldAABB    = Box::Transform(Extent.GetVolume(), Transform.GetWorldspace());
                const IntBox     NewestAABB   = Box::Enclose<SInt32>(WorldAABB) + Transform.GetOrigin();
                const IntVector2 NewestCenter = NewestAABB.GetCenter().GetXZ();

                if (Enclosure.IsLinked())
                {
                    mSupervisor.UpdateEntityOnCell(Actor, Enclosure, NewestCenter);
                }
                else
                {
                    mSupervisor.InsertEntityOnCell(Actor, Enclosure, NewestCenter);
                }
                Enclosure.SetVolume(NewestAABB);
            });

        // System that optimizes the entity hierarchy.
        Scene.CreateSystem<>(
            "World::UpdateHierarchy",
            EcsPostUpdate,
            Scene::Execution::Immediate,
            [this]
            {
                mSupervisor.UpdateHierarchy();
            });

        /// System that disposes of entities marked for disposal.
        Scene.CreateSystem<Scene::DSL::In<Dispose>, Scene::DSL::Read<Enclosure>>(
            "World::DestroyEntitiesTagged",
            EcsPostFrame,
            Scene::Execution::Default,
            [this](Scene::Entity Actor)
            {
                mSupervisor.DetachEntityOnCell(Actor);
                Actor.Destruct();
            });

        // System for clearing stale tags from entities after processing.
        Scene.CreateSystem(
            "World::ClearStaleTagFromEntities",
            EcsPostFrame,
            Scene::Execution::Immediate,
            [this]
            {
                GetService<Scene::Service>().GetWorld().Purge<Stale>();
            });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void World::OnTeardown(Ref<Scene::Service> Scene)
    {
        mSupervisor.Teardown();
    }
}