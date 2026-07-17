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

#include "Tileon.Editor/Activity.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::View
{
    /// \brief Presents the world's singleton components as though the world itself were an entity.
    class Realm final : public Activity
    {
    public:

        /// \brief Constructs the activity with the specified context.
        ///
        /// \param Context The context associated with this activity.
        Realm(Ref<Context> Context);

        /// \see Activity::OnDraw(Ref<UI::Composer>)
        void OnDraw(Ref<UI::Composer> Composer) override;

    private:

        /// \brief Defines the change a row asked for, deferred until the list has finished drawing.
        enum class Action : UInt8
        {
            None,   ///< No change was requested.
            Add,    ///< Attach the subject to the world.
            Remove, ///< Detach the subject from the world.
        };

        /// \brief Draws one singleton the world currently holds.
        ///
        /// \param Composer  The UI composer used to render the component.
        /// \param Component The singleton component to draw.
        void DrawComponent(Ref<UI::Composer> Composer, Scene::Entity Component);

        /// \brief Draws the catalog of singletons the world does not hold yet.
        ///
        /// \param Composer The UI composer used to render the catalog.
        void DrawCatalog(Ref<UI::Composer> Composer);

        /// \brief Draws a placeholder shown when the world holds no singletons at all.
        ///
        /// \param Composer The UI composer used to render the placeholder.
        /// \param Message  The message to display.
        void DrawEmptyPanel(Ref<UI::Composer> Composer, Text Message);

        /// \brief Applies the change a row asked for.
        void Apply();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Registry> mRegistry;
        Action        mAction;
        Scene::Entity mSubject;
    };
}
