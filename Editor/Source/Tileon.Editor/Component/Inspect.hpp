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

#include "Tileon.Editor.UI/Composer.hpp"
#include "Tileon.Render/Component.hpp"
#include "Tileon.World/Component.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// Including it here would cycle back through `Registry`, which `Context` holds by value.
    class Context;

    /// \brief Draws the editable fields of an anchor component.
    ///
    /// \param Composer  The UI composer used to render the fields of the component.
    /// \param Context   The editor context, supplying the services an inspector may need.
    /// \param Actor     The entity that owns the component being inspected.
    /// \param Component The component to inspect.
    /// \return `true` if the user modified the component, `false` otherwise.
    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Anchor> Component);

    /// \brief Draws the editable fields of an extent component.
    ///
    /// \param Composer  The UI composer used to render the fields of the component.
    /// \param Context   The editor context, supplying the services an inspector may need.
    /// \param Actor     The entity that owns the component being inspected.
    /// \param Component The component to inspect.
    /// \return `true` if the user modified the component, `false` otherwise.
    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Extent> Component);

    /// \brief Draws the editable fields of a pose component.
    ///
    /// \param Composer  The UI composer used to render the fields of the component.
    /// \param Context   The editor context, supplying the services an inspector may need.
    /// \param Actor     The entity that owns the component being inspected.
    /// \param Component The component to inspect.
    /// \return `true` if the user modified the component, `false` otherwise.
    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Pose> Component);

    /// \brief Draws the editable fields of a velocity component.
    ///
    /// \param Composer  The UI composer used to render the fields of the component.
    /// \param Context   The editor context, supplying the services an inspector may need.
    /// \param Actor     The entity that owns the component being inspected.
    /// \param Component The component to inspect.
    /// \return `true` if the user modified the component, `false` otherwise.
    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Velocity> Component);

    /// \brief Draws the editable fields of a glow light component.
    ///
    /// \param Composer  The UI composer used to render the fields of the component.
    /// \param Context   The editor context, supplying the services an inspector may need.
    /// \param Actor     The entity that owns the component being inspected.
    /// \param Component The component to inspect.
    /// \return `true` if the user modified the component, `false` otherwise.
    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Glowlight> Component);

    /// \brief Draws the editable fields of a spot light component.
    ///
    /// \param Composer  The UI composer used to render the fields of the component.
    /// \param Context   The editor context, supplying the services an inspector may need.
    /// \param Actor     The entity that owns the component being inspected.
    /// \param Component The component to inspect.
    /// \return `true` if the user modified the component, `false` otherwise.
    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Spotlight> Component);

    /// \brief Draws the editable fields of a sky light component.
    ///
    /// \param Composer  The UI composer used to render the fields of the component.
    /// \param Context   The editor context, supplying the services an inspector may need.
    /// \param Actor     The entity that owns the component being inspected.
    /// \param Component The component to inspect.
    /// \return `true` if the user modified the component, `false` otherwise.
    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Skylight> Component);

    /// \brief Draws the editable fields of a sprite component.
    ///
    /// \param Composer  The UI composer used to render the fields of the component.
    /// \param Context   The editor context, supplying the services an inspector may need.
    /// \param Actor     The entity that owns the component being inspected.
    /// \param Component The component to inspect.
    /// \return `true` if the user modified the component, `false` otherwise.
    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Sprite> Component);

    /// \brief Draws the editable frames of an animation component.
    ///
    /// \param Composer  The UI composer used to render the fields of the component.
    /// \param Context   The editor context, supplying the services an inspector may need.
    /// \param Actor     The entity that owns the component being inspected.
    /// \param Component The component to inspect.
    /// \return `true` if the user modified the component, `false` otherwise.
    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Animation> Component);

    /// \brief Draws the editable fields of a typeface component.
    ///
    /// \param Composer  The UI composer used to render the fields of the component.
    /// \param Context   The editor context, supplying the services an inspector may need.
    /// \param Actor     The entity that owns the component being inspected.
    /// \param Component The component to inspect.
    /// \return `true` if the user modified the component, `false` otherwise.
    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Typeface> Component);

    /// \brief Draws the editable fields of a label component.
    ///
    /// \param Composer  The UI composer used to render the fields of the component.
    /// \param Context   The editor context, supplying the services an inspector may need.
    /// \param Actor     The entity that owns the component being inspected.
    /// \param Component The component to inspect.
    /// \return `true` if the user modified the component, `false` otherwise.
    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<Label> Component);

    /// \brief Draws the editable fields of a tint component.
    ///
    /// \param Composer  The UI composer used to render the fields of the component.
    /// \param Context   The editor context, supplying the services an inspector may need.
    /// \param Actor     The entity that owns the component being inspected.
    /// \param Component The component to inspect.
    /// \return `true` if the user modified the component, `false` otherwise.
    Bool Inspect(Ref<UI::Composer> Composer, Ref<Context> Context, Scene::Entity Actor, Ref<IntColor8> Component);
}
