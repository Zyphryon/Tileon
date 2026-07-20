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

#include "Repository.hpp"
#include "Component/State/Lifecycle.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    static Bool IsOverridable(Scene::Entity Component)  // TODO: Core
    {
        const Scene::Entity::Handle Handle = Component.GetHandle();

        return !Handle.has(flecs::OnInstantiate, flecs::Inherit)
            && !Handle.has(flecs::OnInstantiate, flecs::DontInherit);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Repository::Repository(Ref<Engine::Subsystem::Host> Host)
        : Locator { Host }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repository::Load()
    {
        Ref<Content::Service> Content = GetService<Content::Service>();

        Content.Read(kManifestUri, [this](Filesystem::Result Result, Blob Data)
        {
            GetService<Job::Service>().SubmitOnMain([this, Result, Data = Move(Data)] mutable
            {
                LoadManifest(Result, Move(Data));
            });
        });
        Content.Read(kArchetypeUri, [this](Filesystem::Result Result, Blob Data)
        {
            GetService<Job::Service>().SubmitOnMain([this, Result, Data = Move(Data)] mutable
            {
                LoadArchetypeDatabase(Result, Move(Data));
            });
        });
        Content.Read(kTerrainUri, [this](Filesystem::Result Result, Blob Data)
        {
            GetService<Job::Service>().SubmitOnMain([this, Result, Data = Move(Data)] mutable
            {
                LoadTerrainDatabase(Result, Move(Data));
            });
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repository::Save()
    {
        SaveManifest();
        SaveArchetypeDatabase();
        SaveTerrainDatabase();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Archetype Repository::CreateArchetype()
    {
        return GetService<Scene::Service>().CreateArchetype();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repository::DeleteArchetype(Scene::Archetype Archetype)
    {
        ZY_ASSERT(Archetype.IsValid(), "Must be a valid archetype.");

        // Make sure to remove all instances of the archetype we're removing.
        GetService<Scene::Service>().Defer([Archetype]
        {
            Archetype.Children([](Scene::Entity Instance)
            {
                Instance.Add<Dispose>();
            });
        });

        if (const Scene::Archetype Parent = Archetype.GetParent(); Parent.IsValid())
        {
            Parent.Detach(Archetype);
        }
        else
        {
            Archetype.Destruct();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repository::AttachArchetype(Scene::Archetype Parent, Scene::Archetype Part)
    {
        ZY_ASSERT(Parent.IsValid(), "Must be a valid archetype.");
        ZY_ASSERT(Part.IsValid(), "Must be a valid archetype.");

        Parent.Attach(Part);

        GetService<Scene::Service>().Defer([this, Parent, Part]
        {
            // Re-parenting leaves the old instances hanging under the wrong host, drop them before respawning.
            Part.Children([](Scene::Entity Instance)
            {
                Instance.Add<Dispose>();
            });

            Ref<Scene::Service> Scene = GetService<Scene::Service>();

            Parent.Children([&Scene, Part](Scene::Entity Instance)
            {
                Scene.CreateEntity()
                    .SetName(Part.GetName())
                    .SetArchetype(Part.GetEntity())
                    .Attach(Instance, Scene::Hierarchy::Fixed);
            });
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repository::RefreshArchetype(Scene::Archetype Archetype)
    {
        ZY_ASSERT(Archetype.IsValid(), "Must be a valid archetype.");

        Archetype.Invalidate();

        GetService<Scene::Service>().Defer([Archetype]
        {
            Archetype.Children([Archetype](Scene::Entity Instance)
            {
                // An instance only ever receives copies at the moment its archetype link is created.
                for (Scene::Archetype Base = Archetype; Base.IsValid(); Base = Base.GetArchetype())
                {
                    Base.GetEntity().Each([Instance](Scene::Entity Component)
                    {
                        // TODO: Fix Flecs IsPair
                        if (ecs_id_is_pair(Component.GetID()) || !IsOverridable(Component) || Instance.Owns(Component))
                        {
                            return;
                        }
                        Instance.Add(Component);
                    });
                }
                Instance.Add<Stale>();
            });
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ref<Terrain> Repository::CreateTerrain()
    {
        const UInt32 ID = mTerrains.AllocateWithHandle();
        return mTerrains[ID];
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ref<Terrain> Repository::CloneTerrain(UInt16 ID)
    {
        ZY_ASSERT(HasTerrain(ID), "Must be an existing terrain.");

        const UInt32 Handle = mTerrains.AllocateWithHandle(mTerrains[ID]);
        return mTerrains[Handle];
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repository::DeleteTerrain(UInt16 ID)
    {
        mTerrains.Free(ID);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repository::LoadManifest(Filesystem::Result Result, AnyRef<Blob> Data)
    {
        if (Result == Filesystem::Result::Success)
        {
            Reader Input(Data.GetData(), Data.GetSize());
            GetService<Scene::Service>().LoadWorld(Input);
        }
        else
        {
            LOG_W("Failed to load manifest from '{}'", kManifestUri);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repository::SaveManifest()
    {
        Writer Output;
        GetService<Scene::Service>().SaveWorld(Output);

        GetService<Content::Service>().Write(kManifestUri, Output.Detach(), { });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repository::LoadArchetypeDatabase(Filesystem::Result Result, AnyRef<Blob> Data)
    {
        if (Result == Filesystem::Result::Success)
        {
            Reader Input(Data.GetData(), Data.GetSize());
            GetService<Scene::Service>().LoadArchetypes(Input);
        }
        else
        {
            LOG_W("Failed to load archetypes from '{}'", kArchetypeUri);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repository::SaveArchetypeDatabase()
    {
        Writer Output;
        GetService<Scene::Service>().SaveArchetypes(Output);

        GetService<Content::Service>().Write(kArchetypeUri, Output.Detach(), { });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repository::LoadTerrainDatabase(Filesystem::Result Result, AnyRef<Blob> Data)
    {
        if (Result == Filesystem::Result::Success)
        {
            Reader  Input(Data.GetData(), Data.GetSize());
            Archive(Input).Serialize(mTerrains);
        }
        else
        {
            LOG_W("Failed to load terrains from '{}'", kTerrainUri);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repository::SaveTerrainDatabase()
    {
        Writer  Output;
        Archive(Output).Serialize(mTerrains);

        GetService<Content::Service>().Write(kTerrainUri, Output.Detach(), { });
    }
}