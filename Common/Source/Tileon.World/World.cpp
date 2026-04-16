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
        Scene.GetComponent<EcsPersist>("Persist");
        Scene.GetComponent<EcsDispose>("Dispose");
        Scene.GetComponent<EcsStale>("Stale");
        Scene.GetComponent<EcsKinetic>("Kinetic");
        Scene.GetComponent<Volume>("Volume");
        Scene.GetComponent<Worldspace>("Worldspace").With<Volume>();
        Scene.GetComponent<Localspace>("Localspace").With<Worldspace>().AddTrait(Scene::Trait::Serializable);
        Scene.GetComponent<Origin>("Origin").AddTrait(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Extent>("Extent").AddTrait(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Velocity>("Velocity").With<EcsKinetic>();
        Scene.GetComponent<Region>("Region").AddTrait(Scene::Trait::Serializable);

        // Observe changes to the origin component and mark entities as stale if they are not kinetic.
        Scene.CreateObserver<Scene::DSL::With<Origin>, Scene::DSL::Not<EcsKinetic>>(
            "World::OnSetOriginMarkDirty",
            EcsOnSet,
            [](Scene::Entity Actor)
            {
                Actor.Add<EcsStale>();
            });

        // Observe changes to the extent component and mark entities as stale if they are not kinetic.
        Scene.CreateObserver<Scene::DSL::With<Extent>, Scene::DSL::Not<EcsKinetic>>(
            "World::OnSetDimensionMarkDirty",
            EcsOnSet,
            [](Scene::Entity Actor)
            {
                Actor.Add<EcsStale>();
            });

        // System that propagates dirty state to children (for static entities).
        Scene.CreateSystem<Scene::DSL::Not<EcsStale>, Scene::DSL::Cascade<EcsStale>, Scene::DSL::Out<EcsStale>>(
            "World::PropagateDirtyStates",
            EcsPreUpdate,
            Scene::Execution::Concurrent,
            [](Scene::Entity Actor)
            {
                Actor.Add<EcsStale>();
            });

        // System that computes motion integration.
        Scene.CreateSystem<Scene::DSL::In<const Time, const Velocity, Localspace>>(
            "World::ComputeMotion",
            EcsPreUpdate,
            Scene::Execution::Concurrent,
            [](Scene::Entity Actor, Time Time, ConstRef<Velocity> Velocity, Ref<Localspace> Transform)
            {
                Transform.Translate(Velocity.GetLinear() * Time.GetDelta());
                Transform.Rotate(Velocity.GetAngular() * Time.GetDelta());
            });

        // System that computes world matrices from local transforms of kinetic entities.
        Scene.CreateSystem<Scene::DSL::Cascade<ConstPtr<Worldspace>>, Scene::DSL::In<const Localspace, ConstPtr<Origin>, Worldspace>, EcsDynamic>(
            "World::ComputeWorldspace",
            EcsOnUpdate,
            Scene::Execution::Concurrent,
            [](ConstPtr<Worldspace> Parent, ConstRef<Localspace> Localspace, ConstPtr<Origin> Origin, Ref<Worldspace> Worldspace)
            {
                const Vector2 Offset = Origin ? (* Origin) : Vector2::Zero();
                Worldspace = Parent ? (* Parent) * Localspace.Compute(Offset) : Localspace.Compute(Offset);
            });

        // System that computes world-space volumes from local-space volumes and updates spatial partitioning.
        Scene.CreateSystem<Scene::DSL::In<const Worldspace, const Extent, Volume>, EcsDynamic>(
            "World::ComputeHierarchy",
            EcsOnUpdate,
            Scene::Execution::Concurrent,
            [this](Scene::Entity Actor, ConstRef<Worldspace> Worldspace, Extent Extent, Ref<Volume> Volume)
            {
                const Rect LocalAABB       = Rect(Extent.GetOffset(), Extent.GetOffset() + Extent.GetSize());
                const Rect NewestWorldAABB = Rect::Transform(LocalAABB, Worldspace);

                if (Volume.IsAlmostZero())
                {
                    mSupervisor.InsertEntityOnCell(Actor, NewestWorldAABB);
                }
                else
                {
                    mSupervisor.UpdateEntityOnCell(Actor, Volume, NewestWorldAABB);
                }
                Volume = NewestWorldAABB;
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
        Scene.CreateSystem<Scene::DSL::In<ConstPtr<Volume>, const EcsDispose>>(
            "World::DestroyEntitiesTagged",
            EcsPostFrame,
            Scene::Execution::Concurrent,
            [this](Scene::Entity Actor, ConstPtr<Volume> Volume)
            {
                if (Volume)
                {
                    mSupervisor.RemoveEntityOnCell(Actor, * Volume);
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
                GetService<Scene::Service>().Clear<EcsStale>();
            });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void World::OnTeardown(Ref<Scene::Service> Scene)
    {
        // Forward teardown to supervisor sub module.
        mSupervisor.Teardown();
    }
}