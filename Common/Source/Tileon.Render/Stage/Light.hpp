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
#include <Zyphryon.Content/Service.hpp>
#include <Zyphryon.Graphic/Technique.hpp>
#include <Zyphryon.Scene/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Stage
{
    /// \brief Represents the light stage of the rendering pipeline, responsible for applying lighting effects to the scene.
    class Light final
    {
    public:

        /// \brief Constructs the stage instance with the specified service host.
        ///
        /// \param Host The service host to associate with the stage.
        Light(Ref<Engine::Subsystem::Host> Host);

        /// \brief Executes the stage's main logic.
        ///
        /// \param Graphics The graphic service used to submit draw calls for this stage.
        /// \param Director The director instance providing camera and view management for rendering.
        /// \param Normal   The normal map object used for lighting calculations in the stage.
        void Run(Ref<Graphic::Service> Graphics, ConstRef<Director> Director, Graphic::Object Normal);

    private:

        /// \brief Enumerates the different rendering techniques available in the light stage.
        enum class Kind
        {
            Spotlight, ///< Technique for rendering cone-shaped light sources.
            Glowlight, ///< Technique for rendering radial light sources.
            Skylight,  ///< Technique for rendering ambient lighting effects that affect the entire scene.
        };

        /// \brief Defines a type alias for a collection of rendering techniques.
        using Techniques = Array<Retainer<Graphic::Technique>, Enum::Count<Kind>()>;

        /// \brief Registers the stage with the specified scene service.
        ///
        /// \param Scene The scene service to register with.
        void OnRegister(Ref<Scene::Service> Scene);

        /// \brief Loads the necessary resources for the stage, such as shaders and materials.
        ///
        /// \param Content The content service used to load resources for the stage.
        void OnLoad(Ref<Content::Service> Content);

        /// \brief Represents the per-instance data for the ambient lighting effect of the skylight technique.
        struct GpuSkylightLayout final
        {
            /// The color of the sun, represented as RGB + intensity, A = SunDirection.X.
            Color SunColor;

            /// The color of the sky, represented as RGB + intensity, A = SunDirection.Y.
            Color SkyColor;

            /// The color of the ground, represented as RGB + intensity.
            Color GroundColor;
        };

        /// \brief Represents the per-instance data for a glow light.
        struct GpuGlowlightLayout final
        {
            /// The center position of the radial light in world space.
            Vector2 Center;

            // The range of the radial light, representing the maximum distance the light can reach.
            Real32  Radius;

            // The falloff of the radial light, representing the rate at which the light intensity decreases with distance.
            Real32  Falloff;

            // The color of the radial light, represented as RGB + intensity.
            Color   Color;
        };

        /// \brief Represents the per-instance data for a spot light.
        struct GpuSpotlightLayout final
        {
            /// The center position of the cone light in world space.
            Vector2   Center;

            // The range of the cone light, representing the maximum distance the light can reach.
            Real32    Range;

            // The falloff of the cone light, representing the rate at which the light intensity decreases with distance.
            Real32    Falloff;

            // The direction of the cone light,indicating the forward direction of the light.
            Vector2   Direction;

            // The cos of the inner and outer angles of the cone light.
            Vector2   Angles;

            // The color of the cone light, represented as RGB + intensity.
            Color     Color;
        };

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Techniques                   mTechniques;
        Sequence<GpuGlowlightLayout> mGlowlightData;
        Sequence<GpuSpotlightLayout> mSpotlightData;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Scene::Query                 mQrDrawGlowlights;
        Scene::Query                 mQrDrawSpotlights;
        Scene::Query                 mQrDrawSkylight;
    };
}