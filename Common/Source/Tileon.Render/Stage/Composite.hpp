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

#include <Zyphryon.Content/Service.hpp>
#include <Zyphryon.Engine/Locator.hpp>
#include <Zyphryon.Graphic/Technique.hpp>
#include <Zyphryon.Math/Vector4.hpp>
#include <Zyphryon.Render/Pass.hpp>
#include <Zyphryon.Scene/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Stage
{
    /// \brief Represents the composite stage of the rendering pipeline, responsible for compositing the final image.
    class Composite final : public Render::Pass, public Engine::Locator<Graphic::Service>
    {
    public:

        /// \brief Constructs the stage instance with the specified service host.
        ///
        /// \param Host     The service host to associate with the stage.
        /// \param Albedo   The target holding the scene's base color, sampled by the stage.
        /// \param Radiance The target holding the scene's accumulated lighting, sampled by the stage.
        Composite(Ref<Engine::Subsystem::Host> Host, ConstRef<Render::Target> Albedo, ConstRef<Render::Target> Radiance);

        /// \brief Executes the stage's main logic.
        ///
        /// \param Encoder The render encoder used to submit draw calls for this stage.
        void Run(Ref<Render::Encoder> Encoder) override;

    private:

        /// \brief Enumerates the different rendering techniques available in the composite stage.
        enum class Kind
        {
            Composite,  ///< Technique for compositing the final image.
        };

        /// \brief Defines a type alias for a collection of rendering techniques.
        using Techniques = Array<Retainer<Graphic::Technique>, Enum::Count<Kind>()>;

        /// \brief Represents the per-pass data the composite technique consumes.
        struct GpuCompositeLayout final
        {
            /// The exposure applied before the tone map, with the remaining channels unused.
            Vector4 Exposure;
        };

        /// \brief Loads the necessary resources for the stage, such as shaders and materials.
        ///
        /// \param Content The content service used to load resources for the stage.
        void OnLoad(Ref<Content::Service> Content);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Scene::World             mWorld;
        Techniques               mTechniques;
        ConstPtr<Render::Target> mAlbedo;
        ConstPtr<Render::Target> mRadiance;
    };
}