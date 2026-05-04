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

#include "Stage/Geometry.hpp"
#include "Stage/Light.hpp"
#include "Stage/Composite.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Visual
{
    /// \brief Represents the renderer responsible for rendering the scene using a deferred rendering pipeline.
    class Renderer final : public Locator<Graphic::Service, Content::Service>
    {
    public:

        /// \brief Represents the different types of frames used in the rendering pipeline.
        enum class Frame : UInt8
        {
            Albedo,         ///< Contains the base color information of the scene.
            Normal,         ///< Contains the surface normal information of the scene for lighting calculations.
            Depth,          ///< Contains the depth information of the scene for depth testing and effects.
            Radiance,       ///< Contains the accumulated lighting information of the scene after the lighting phase.
            Final,          ///< Contains the final composed image of the scene after all rendering phases are complete.
        };

        /// \brief Represents the different phases of the rendering pipeline.
        enum class Phase : UInt8
        {
            Geometry,       ///< The geometry phase, where the scene's geometry is rendered to the G-buffer.
            Light,          ///< The lighting phase, where lighting calculations are performed using the G-buffer data.
            Composite,      ///< The composition phase, where the final image is composed by combining previous phases.
        };

    public:

        /// \brief Constructs a renderer with the specified service host.
        ///
        /// \param Host The service host to associate with the renderer.
        Renderer(Ref<Service::Host> Host);

        /// \brief Gets the tileset associated with the renderer, which manages the rendering data for different terrains.
        ///
        /// \return The tileset associated with the renderer.
        ZYPHRYON_INLINE Ref<Tileset> GetTileset()
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
        /// \param Director  The director that holds projection and view information for rendering.
        /// \param Immediate If `true`, renders directly to the display; if `false`, renders to an off-screen texture.
        void Present(ConstRef<Director> Director, Bool Immediate);

        /// \brief Gets the GPU texture for the specified frame slot.
        ///
        /// \param Type The type of frame to retrieve (e.g., Albedo, Normal, Depth).
        /// \return The GPU texture handle for the requested frame, valid until the next resize call.
        ZYPHRYON_INLINE Graphic::Object GetFrame(Frame Type) const
        {
            return mFrames[Enum::Cast(Type)];
        }

    private:

        /// \brief Represents the rendering pipeline, which manages the sequence of rendering stages.
        struct Pipeline
        {
            /// \brief The geometry stage of the pipeline, responsible for rendering scene geometry.
            Stage::Geometry  Geometry;

            /// \brief The lighting stage of the pipeline, responsible for applying lighting effects to the scene.
            Stage::Light     Light;

            /// \brief The composition stage of the pipeline, responsible for composing the final image.
            Stage::Composite Composite;

            /// \brief Constructs the pipeline with the specified service host.
            ///
            /// \param Host The service host to associate with the pipeline and its stages.
            ZYPHRYON_INLINE Pipeline(Ref<Service::Host> Host)
                : Geometry  { Host },
                  Light     { Host },
                  Composite { Host }
            {
            }
        };

        /// \brief Registers the renderer with the specified scene service.
        ///
        /// \param Scene The scene service to register with.
        void OnRegister(Ref<Scene::Service> Scene);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Graphic::Encoder                             mEncoder;
        Graphic::Viewport                            mViewport;
        Array<Graphic::Object, Enum::Count<Phase>()> mPhases;
        Array<Graphic::Object, Enum::Count<Frame>()> mFrames;
        Pipeline                                     mPipeline;
        Tileset                                      mTileset;
    };
}