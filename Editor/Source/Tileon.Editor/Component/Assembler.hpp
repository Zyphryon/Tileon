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

#include "Tileon.Editor/Context.hpp"
#include "Tileon.Editor/UI/Composer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Draws the component list of an entity, allowing components to be added, edited and removed.
    class Assembler final
    {
    public:

        /// \brief Constructs an assembler backed by the specified context.
        ///
        /// \param Context The context supplying the catalog of authorable components and the services inspectors need.
        explicit Assembler(Ref<Context> Context);

        /// \brief Draws the component list of an entity.
        ///
        /// \param Composer The UI composer used to render the component list.
        /// \param Actor    The entity whose components are being edited.
        void Draw(Ref<UI::Composer> Composer, Scene::Entity Actor);

        /// \brief Draws the asset browser a field opened, if any.
        ///
        /// Kept apart from \ref Draw because the browser is modal and belongs at window scope, while the component
        /// list is drawn inside a child. Views call this after closing the child that hosts the list.
        ///
        /// \param Composer The UI composer used to render the browser.
        ZY_INLINE void DrawSelector(Ref<UI::Composer> Composer)
        {
            mSelector.Draw(Composer);
        }

    private:

        /// \brief Defines the structural changes that can be requested while drawing.
        enum class Action : UInt8
        {
            None,       ///< No structural change was requested.
            Add,        ///< Attach a component to the entity.
            Remove,     ///< Detach a component from the entity.
            Override,   ///< Copy an inherited component down onto the entity.
        };

        /// \brief Checks whether a component belongs on an entity, given whether that entity is an archetype.
        ///
        /// \param Actor     The entity the component would be drawn on.
        /// \param Component The component entity to test.
        /// \return `true` if the component is inspectable and in scope for the entity, `false` otherwise.
        Bool IsAuthorable(Scene::Entity Actor, Scene::Entity Component) const;

        /// \brief Draws a single component as a collapsible section.
        ///
        /// \param Composer  The UI composer used to render the component.
        /// \param Actor     The entity that owns or inherits the component.
        /// \param Component The component entity being drawn.
        /// \param Inherited `true` if the component's value comes from the entity's archetype.
        void DrawComponent(Ref<UI::Composer> Composer, Scene::Entity Actor, Scene::Entity Component, Bool Inherited);

        /// \brief Draws the popup listing every component that can be attached to the entity.
        ///
        /// \param Composer The UI composer used to render the catalog.
        /// \param Actor    The entity the component would be attached to.
        void DrawCatalog(Ref<UI::Composer> Composer, Scene::Entity Actor);

        /// \brief Applies the structural change recorded while drawing, if any.
        ///
        /// \param Actor The entity to apply the change to.
        void Apply(Scene::Entity Actor);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Context>  mContext;
        Ref<Registry> mRegistry;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UI::Selector  mSelector;
        Action        mAction;
        Scene::Entity mSubject;
    };
}
