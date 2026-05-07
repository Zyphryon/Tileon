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

#include "Tileon.Render/Director.hpp"
#include "Tileon.Render/Renderer.hpp"
#include "Tileon.World/World.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents the controller responsible for coordinating the world.
    class Controller final : public Locator<Scene::Service>
    {
        // TODO: Split into common, client and server orquestrator

    public:

        /// \brief Constructs a Controller instance with the specified service host.
        ///
        /// \param Host The service host to associate with the controller.
        explicit Controller(Ref<Service::Host> Host);

        /// \brief Tears down the controller, releasing any allocated resources.
        void Teardown();

        /// \brief Initializes the world and related systems with the specified parameters.
        ///
        /// \param Width   The display width (in pixels) for rendering the world.
        /// \param Height  The display height (in pixels) for rendering the world.
        /// \param Density The pixel density that defines how many pixels one logical world unit occupies on screen.
        void Init(UInt16 Width, UInt16 Height, UInt16 Density);

        /// \brief Loads the world and initializes necessary resources for rendering.
        void Load();

        /// \brief Saves the current state of the world and any relevant data.
        void Save();

        /// \brief Resizes the rendering viewport to the specified dimensions.
        ///
        /// \param Width  The new width of the viewport in pixels.
        /// \param Height The new height of the viewport in pixels.
        void Resize(UInt16 Width, UInt16 Height);

        /// \brief Presents the rendered scene to the display, executing the rendering pipeline.
        ///
        /// \param Immediate If `true`, renders directly to the display; if `false`, renders to an off-screen texture.
        void Present(Bool Immediate);

        /// \brief Gets the world instance associated with the controller.
        ///
        /// \return The world instance managed by the controller.
        ZYPHRYON_INLINE Ref<World> GetWorld()
        {
            return mWorld;
        }

        /// \brief Gets the director instance associated with the controller.
        ///
        /// \return The director instance managed by the controller.
        ZYPHRYON_INLINE Ref<Director> GetDirector()
        {
            return mDirector;
        }

        /// \brief Gets the renderer instance associated with the controller.
        ///
        /// \return The renderer instance managed by the controller.
        ZYPHRYON_INLINE Ref<Renderer> GetRenderer()
        {
            return mRenderer;
        }

    private:

        /// \brief Registers the controller with the specified scene service.
        ///
        /// \param Scene The scene service to register with.
        void OnRegister(Ref<Scene::Service> Scene);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        World    mWorld;
        Director mDirector;
        Renderer mRenderer;
    };
}
