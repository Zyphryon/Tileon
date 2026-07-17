// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Descriptor.hpp"
#include <Zyphryon.Scene/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Publishes the set of components the editor knows how to inspect.
    class Registry final
    {
    public:

        /// \brief Constructs a registry and publishes every known component into the world.
        ///
        /// \param Scene The scene service that owns the component entities.
        explicit Registry(Ref<Scene::Service> Scene);

        /// \brief Gets every published component, in registration order.
        ///
        /// \return The catalog of published component entities.
        ZY_INLINE ConstSpan<Scene::Entity> GetCatalog() const
        {
            return mCatalog;
        }

    private:

        /// \brief Publishes a component, making it inspectable and available in the catalog.
        ///
        /// \tparam Type  The component type to publish.
        /// \param  Name  The name the component was registered with. Must match the runtime's registration.
        /// \param  Icon  The glyph displayed alongside the label.
        /// \param  Group The category the component is filed under in the catalog.
        /// \param  Scope The kind of entity the component may be authored on.
        template<typename Type>
        void Add(Text Name, Text Icon, Text Group, Descriptor::Scope Scope = Descriptor::Scope::Any)
        {
            const Scene::Entity Component = mScene.GetComponent<Type>(Name);

            Component.Set(Descriptor::Create<Type>(Name, Icon, Group, Scope));

            mCatalog.Append(Component);
        }

        /// \brief Publishes every component the editor can author.
        void OnRegister();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Scene::Service>     mScene;
        Sequence<Scene::Entity> mCatalog;
    };
}
