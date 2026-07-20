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

#include "Inspect.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Checks whether a component type has an inspection handler.
    ///
    /// \tparam Type The component type to check for an `Inspect` overload.
    template<typename Type>
    concept IsInspectable = requires (
        Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ref<Type> Component)
    {
        Inspect(Composer, Workspace, Actor, Component);
    };

    /// \brief Type-erased inspection dispatcher for a single component type.
    class Descriptor final
    {
    public:

        /// \brief Signature for a component inspection handler.
        using Handler = Bool (*)(Ref<UI::Composer>, Ref<Workspace>, Scene::Entity, Ptr<void>);

        /// \brief Defines the kind of entity a component may be authored on.
        enum class Scope : UInt8
        {
            Any,        ///< Authorable on archetypes and instances alike.
            Instance,   ///< Authorable only on instances, as its value is meaningless to share.
            Archetype,  ///< Authorable only on archetypes.
        };

    public:

        /// \brief Constructs an invalid descriptor with no handler attached.
        ZY_INLINE Descriptor() = default;

        /// \brief Constructs a descriptor with an explicit inspection handler.
        ///
        /// \param Inspector The handler invoked when drawing the component, or `nullptr` for a data-less tag.
        /// \param Label     The human-readable name displayed for the component.
        /// \param Icon      The glyph displayed alongside the label.
        /// \param Group     The category the component is filed under in the catalog.
        /// \param Scope     The kind of entity the component may be authored on.
        ZY_INLINE Descriptor(Handler Inspector, Text Label, Text Icon, Text Group, Scope Scope)
            : mInspector { Inspector },
              mLabel     { Label },
              mIcon      { Icon },
              mGroup     { Group },
              mScope     { Scope }
        {
        }

        /// \brief Checks whether the component may be authored on a given kind of entity.
        ///
        /// \param Archetype `true` to test against an archetype, `false` to test against an instance.
        /// \return `true` if the component belongs on that kind of entity, `false` otherwise.
        ZY_INLINE Bool IsAuthorableOn(Bool Archetype) const
        {
            switch (mScope)
            {
            case Scope::Instance:
                return !Archetype;
            case Scope::Archetype:
                return Archetype;
            default:
                return true;
            }
        }

        /// \brief Draws the component, allowing the user to modify it.
        ///
        /// \param Composer  The UI composer used to render the fields of the component.
        /// \param Workspace The workspace being drawn in, supplying services and the view's widgets.
        /// \param Actor     The entity that owns the component instance.
        /// \param Component The raw pointer to the component instance.
        /// \return `true` if the user modified the component, `false` otherwise.
        ZY_INLINE Bool Inspect(Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ptr<void> Component) const
        {
            return mInspector && Component ? mInspector(Composer, Workspace, Actor, Component) : false;
        }

        /// \brief Checks whether the component carries data the user can edit.
        ///
        /// \return `true` if the component has an inspection handler, `false` if it is a data-less tag.
        ZY_INLINE Bool HasFields() const
        {
            return mInspector != nullptr;
        }

        /// \brief Gets the human-readable name displayed for the component.
        ///
        /// \return The label of the component.
        ZY_INLINE Text GetLabel() const
        {
            return mLabel;
        }

        /// \brief Gets the glyph displayed alongside the label.
        ///
        /// \return The icon of the component.
        ZY_INLINE Text GetIcon() const
        {
            return mIcon;
        }

        /// \brief Gets the category the component is filed under in the catalog.
        ///
        /// \return The group of the component.
        ZY_INLINE Text GetGroup() const
        {
            return mGroup;
        }

    public:

        /// \brief Creates a `Descriptor` bound to a specific component type.
        ///
        /// \param Label  The human-readable name displayed for the component.
        /// \param Icon   The glyph displayed alongside the label.
        /// \param Group  The category the component is filed under in the catalog.
        /// \param Scope  The kind of entity the component may be authored on.
        /// \return A fully initialized `Descriptor` ready to draw instances of `Component`.
        template<typename Component>
        ZY_INLINE static Descriptor Create(Text Label, Text Icon, Text Group, Scope Scope)
        {
            if constexpr (IsInspectable<Component>)
            {
                return Descriptor(OnInspect<Component>, Label, Icon, Group, Scope);
            }
            else
            {
                return Descriptor(nullptr, Label, Icon, Group, Scope);
            }
        }

    private:

        /// \brief Inspection handler generated for `Type` by `Create<Type>()`.
        ///
        /// \param Composer  The UI composer used to render the fields of the component.
        /// \param Workspace The workspace being drawn in, supplying services and the view's widgets.
        /// \param Actor     The entity that owns the component instance.
        /// \param Component The raw pointer to the component instance.
        /// \return `true` if the user modified the component, `false` otherwise.
        template<typename Type>
        ZY_INLINE static Bool OnInspect(Ref<UI::Composer> Composer, Ref<Workspace> Workspace, Scene::Entity Actor, Ptr<void> Component)
        {
            return Editor::Inspect(Composer, Workspace, Actor, * static_cast<Ptr<Type>>(Component));
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Handler mInspector;
        Text    mLabel;
        Text    mIcon;
        Text    mGroup;
        Scope   mScope;
    };
}
