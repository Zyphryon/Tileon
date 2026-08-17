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
#include "Component/Lifecycle.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
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
            GetService<Job::Service>().Dispatch(Job::Lane::Main, [this, Result, Data = Move(Data)] mutable
            {
                LoadManifest(Result, Move(Data));
            });
        });
        Content.Read(kArchetypeUri, [this](Filesystem::Result Result, Blob Data)
        {
            GetService<Job::Service>().Dispatch(Job::Lane::Main, [this, Result, Data = Move(Data)] mutable
            {
                LoadArchetypeDatabase(Result, Move(Data));
            });
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Repository::Save()
    {
        SaveManifest();
        SaveArchetypeDatabase();
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

            Ptr<Scene::Service> Scene = AddressOf(GetService<Scene::Service>());

            Parent.Children([Scene, Part](Scene::Entity Instance)
            {
                Scene->CreateEntity()
                    .SetAlias(Part.GetAlias())
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
                        if (!Component.IsPair() && Component.IsOverridable() && !Instance.Owns(Component))
                        {
                            Instance.Add(Component);
                        }
                    });
                }
                Instance.Add<Stale>();
            });
        });
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
}