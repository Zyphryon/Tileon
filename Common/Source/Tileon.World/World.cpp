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
#include "Component.hpp"

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
        Scene.GetComponent<Persist>("Persist");
        Scene.GetComponent<Dispose>("Dispose");
        Scene.GetComponent<Stale>("Stale");
        Scene.GetComponent<Dynamic>("Dynamic");
        Scene.GetComponent<Worldspace>("Worldspace");
        Scene.GetComponent<Localspace>("Localspace").With<Worldspace>().AddTrait(Scene::Trait::Serializable);
        Scene.GetComponent<Origin>("Origin").AddTrait(Scene::Trait::Serializable, Scene::Trait::Inheritable);
        Scene.GetComponent<Sector>("Sector");
        Scene.GetComponent<Volume>("Volume");
        Scene.GetComponent<Extent>("Extent").AddTrait(Scene::Trait::Serializable, Scene::Trait::Inheritable).With<Volume>();
        Scene.GetComponent<Velocity>("Velocity").With<Dynamic>();
        Scene.GetComponent<Region>("Region").AddTrait(Scene::Trait::Serializable);

        // Observe changes to the origin component and mark entities as stale if they are not kinetic.
        Scene.CreateObserver<Scene::DSL::With<Origin>, Scene::DSL::Not<Dynamic>>(
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

        // System that propagates dirty state to children (for static entities).
        Scene.CreateSystem<Scene::DSL::Not<Stale>, Scene::DSL::Cascade<Stale>, Scene::DSL::Out<Stale>>(
            "World::PropagateDirtyStates",
            EcsPreUpdate,
            Scene::Execution::Concurrent,
            [](Scene::Entity Actor)
            {
                Actor.Add<Stale>();
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
        Scene.CreateSystem<Scene::DSL::Cascade<ConstPtr<Worldspace>>, Scene::DSL::In<const Localspace, ConstPtr<Origin>, Worldspace>, Kinetic>(
            "World::ComputeWorldspace",
            EcsOnUpdate,
            Scene::Execution::Concurrent,
            [](ConstPtr<Worldspace> Parent, ConstRef<Localspace> Localspace, ConstPtr<Origin> Origin, Ref<Worldspace> Worldspace)
            {
                const Vector2 Offset = Origin ? (* Origin) : Vector2::Zero();
                Worldspace = Parent ? (* Parent) * Localspace.Compute(Offset) : Localspace.Compute(Offset);
            });

        // System that computes world-space volumes from local-space volumes and updates spatial partitioning.
        // TODO: Remove Up(Sector)
        Scene.CreateSystem<Scene::DSL::Up<const Sector>, Scene::DSL::In<const Worldspace, const Extent, Volume>, Kinetic>(
            "World::ComputeHierarchy",
            EcsOnUpdate,
            Scene::Execution::Concurrent,
            [this](Scene::Entity Actor, ConstRef<Sector> Sector, ConstRef<Worldspace> Worldspace, Extent Extent, Ref<Volume> Volume)
            {
                const Rect LocalAABB(Extent.GetOffset(), Extent.GetOffset() + Extent.GetSize());
                const Rect WorldAABB = Rect::Transform(LocalAABB, Worldspace);

                const IntRect    NewestVolume = Rect::Enclose<SInt32>(WorldAABB) + Sector;
                const IntVector2 NewestCenter = NewestVolume.GetCenter();

                if (Volume.IsAlmostZero())
                {
                    mSupervisor.InsertEntityOnCell(Actor, NewestCenter);
                }
                else
                {
                    mSupervisor.UpdateEntityOnCell(Actor, Volume.GetCenter(), NewestCenter);
                }
                Volume = NewestVolume;
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

        // TODO: Parent Migration.

        /// System that disposes of entities marked for disposal.
        Scene.CreateSystem<Scene::DSL::In<ConstPtr<Volume>, const Dispose>>(
            "World::DestroyEntitiesTagged",
            EcsPostFrame,
            Scene::Execution::Concurrent,
            [this](Scene::Entity Actor, ConstPtr<Volume> Volume)
            {
                if (Volume)
                {
                   mSupervisor.RemoveEntityOnCell(Actor, Volume->GetCenter());
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