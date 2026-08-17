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
#include <Zyphryon.Engine/Locator.hpp>
#include <Zyphryon.Graphic/Technique.hpp>
#include <Zyphryon.Render/Pass.hpp>
#include <Zyphryon.Scene/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Stage
{
    /// \brief Represents the light stage of the rendering pipeline, responsible for applying lighting effects to the scene.
    class Lighting final : public Render::Pass, public Engine::Locator<Graphic::Service>
    {
    public:

        /// \brief Constructs the stage instance with the specified service host.
        ///
        /// \param Host   The service host to associate with the stage.
        /// \param Normal The target holding the scene's surface normals, sampled by every light technique.
        /// \param Depth  The target holding the scene's depth, which the placed lights unproject into world space.
        Lighting(Ref<Engine::Subsystem::Host> Host, ConstRef<Render::Target> Normal, ConstRef<Render::Target> Depth);

        /// \brief Sets the director the stage resolves its draws against for the current frame.
        ///
        /// \param Director The director instance providing camera and view management for rendering.
        ZY_INLINE void SetDirector(ConstRef<Director> Director)
        {
            mDirector = AddressOf(Director);
        }

        /// \brief Executes the stage's main logic.
        ///
        /// \param Encoder The render encoder used to submit draw calls for this stage.
        void Run(Ref<Render::Encoder> Encoder) override;

    private:

        /// \brief Enumerates the different rendering techniques available in the light stage.
        enum class Kind : UInt8
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

            /// The color of the ground, represented as RGB + intensity, A = SunDirection.Z.
            Color GroundColor;
        };

        /// \brief Represents the per-instance data for a glow light.
        struct GpuGlowlightLayout final
        {
            /// The center position of the radial light in world space.
            Vector3 Center;

            // The range of the radial light, representing the maximum distance the light can reach.
            Real32  Radius;

            // The color of the radial light, represented as RGB + falloff.
            Color   Color;
        };

        /// \brief Represents the per-instance data for a spot light.
        struct GpuSpotlightLayout final
        {
            /// The center position of the cone light in world space.
            Vector3   Center;

            // The range of the cone light, representing the maximum distance the light can reach.
            Real32    Range;

            // The direction the cone points, in world space, so it may be aimed up or down as freely as around.
            Vector3   Direction;

            // The cos of the inner angle, riding alongside the direction to fill out its slot.
            Real32    Inner;

            // The color of the cone light, represented as RGB + falloff.
            Color     Color;

            // The cos of the outer angle.
            Real32    Outer;
        };

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Techniques                   mTechniques;
        Sequence<GpuGlowlightLayout> mGlowlightData;
        Sequence<GpuSpotlightLayout> mSpotlightData;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        ConstPtr<Render::Target>     mNormal;
        ConstPtr<Render::Target>     mDepth;
        ConstPtr<Director>           mDirector;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Scene::Query                 mQrDrawGlowlights;
        Scene::Query                 mQrDrawSpotlights;
        Scene::Query                 mQrDrawSkylight;
    };
}