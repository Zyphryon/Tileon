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

#include "Director.hpp"
#include "Tileset.hpp"
#include <Zyphryon.Content/Service.hpp>
#include <Zyphryon.Engine/Locator.hpp>
#include <Zyphryon.Render/Renderer.hpp>
#include <Zyphryon.Scene/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents the renderer responsible for rendering the scene using a deferred rendering pipeline.
    class Renderer final : public Engine::Locator<Graphic::Service, Content::Service>
    {
    public:

        /// \brief Represents the different types of targets used in the rendering pipeline.
        enum class Target : UInt8
        {
            Albedo,         ///< Contains the base color information of the scene.
            Normal,         ///< Contains the surface normal information of the scene for lighting calculations.
            Depth,          ///< Contains the depth information of the scene for depth testing and effects.
            Radiance,       ///< Contains the accumulated lighting information of the scene after the lighting phase.
            Final,          ///< Contains the final composed image of the scene after all rendering phases are complete.
        };

    public:

        /// \brief Constructs a renderer with the specified service host.
        ///
        /// \param Host      The service host to associate with the renderer.
        /// \param Immediate `true` composes into the display otherwise composes into \ref Target::Final.
        Renderer(Ref<Engine::Subsystem::Host> Host, Bool Immediate);

        /// \brief Gets the tileset associated with the renderer.
        ///
        /// \return The tileset associated with the renderer.
        ZY_INLINE Ref<Tileset> GetTileset()
        {
            return mTileset;
        }

        /// \brief Loads necessary resources and initializes the renderer.
        void Load();

        /// \brief Saves any necessary data or state related to the renderer.
        void Save();

        /// \brief Resizes the rendering viewport to the specified dimensions.
        ///
        /// \param Width  The new width of the viewport in pixels.
        /// \param Height The new height of the viewport in pixels.
        void Resize(UInt16 Width, UInt16 Height);

        /// \brief Executes the rendering pipeline to compose the scene.
        ///
        /// \param Director The director that holds projection and view information for rendering.
        void Present(ConstRef<Director> Director);

        /// \brief Gets the GPU texture for the specified frame slot.
        ///
        /// \param Type The type of target to retrieve (e.g., Albedo, Normal, Depth).
        /// \return The GPU texture handle for the requested target, valid until the next resize call.
        ZY_INLINE Graphic::Object GetTarget(Target Type) const
        {
            return mRenderer.GetTarget(Enum::Cast(Type)).GetTexture();
        }

    private:

        /// \brief Declares the pipeline's managed targets and its stages, in execution order.
        ///
        /// \param Host      The service host to associate with each stage.
        /// \param Immediate If `true`, the composite stage targets the display instead of \ref Target::Final.
        void OnCreate(Ref<Engine::Subsystem::Host> Host, Bool Immediate);

        /// \brief Registers the renderer with the specified scene service.
        ///
        /// \param Scene The scene service to register with.
        void OnRegister(Ref<Scene::Service> Scene);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Render::Renderer mRenderer;
        Tileset          mTileset;
    };
}
