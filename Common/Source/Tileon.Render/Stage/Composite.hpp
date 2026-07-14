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
#include <Zyphryon.Graphic/Technique.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Stage
{
    /// \brief Represents the composite stage of the rendering pipeline, responsible for compositing the final image.
    class Composite final
    {
    public:

        /// \brief Constructs the stage instance with the specified service host.
        ///
        /// \param Host The service host to associate with the stage.
        Composite(Ref<Engine::Subsystem::Host> Host);

        /// \brief Executes the stage's main logic.
        ///
        /// \param Graphics The graphic service used to submit draw calls for this stage.
        /// \param Albedo   The albedo map object containing the base color information for the scene.
        /// \param Radiance The radiance map object containing the lighting information for the scene.
        void Run(Ref<Graphic::Service> Graphics, Graphic::Object Albedo, Graphic::Object Radiance);

    private:

        /// \brief Enumerates the different rendering techniques available in the composite stage.
        enum class Kind
        {
            Composite,  ///< Technique for compositing the final image.
        };

        /// \brief Defines a type alias for a collection of rendering techniques.
        using Techniques = Array<Retainer<Graphic::Technique>, Enum::Count<Kind>()>;

        /// \brief Loads the necessary resources for the stage, such as shaders and materials.
        ///
        /// \param Content The content service used to load resources for the stage.
        void OnLoad(Ref<Content::Service> Content);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Techniques mTechniques;
    };
}