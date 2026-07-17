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
    /// \brief Represents the debug stage of the rendering pipeline, responsible for drawing diagnostic overlays.
    class Debug final : public Render::Pass, public Engine::Locator<Graphic::Service>
    {
    public:

        /// \brief Enumerates the diagnostic overlays that can be drawn by the stage.
        enum class Property : UInt8
        {
            Boundaries = 0b00000001,    ///< Draws the world-space bounding box of every visible entity.
            Grid       = 0b00000010,    ///< Draws the infinite tile grid, accented on region borders.
        };

    public:

        /// \brief Constructs the stage instance with the specified service host.
        ///
        /// \param Host The service host to associate with the stage.
        explicit Debug(Ref<Engine::Subsystem::Host> Host);

        /// \brief Sets the director the stage resolves its draws against for the current frame.
        ///
        /// \param Director The director instance providing camera and view management for rendering.
        ZY_INLINE void SetDirector(ConstRef<Director> Director)
        {
            mDirector = AddressOf(Director);
        }

        /// \brief Changes the state of a specific property for the stage.
        ///
        /// \param Mask   The property to set or clear.
        /// \param Enable `true` to set the property, `false` to clear it.
        ZY_INLINE void SetProperty(Property Mask, Bool Enable)
        {
            mProperties = SetOrClearBit(mProperties, Enum::Cast(Mask), Enable);
        }

        /// \brief Checks if the stage has a specific property.
        ///
        /// \param Mask The property to check.
        /// \return `true` if the property is enabled, `false` otherwise.
        ZY_INLINE Bool HasProperty(Property Mask) const
        {
            return HasBit(mProperties, Enum::Cast(Mask));
        }

        /// \brief Executes the stage's main logic.
        ///
        /// \param Encoder The render encoder used to submit draw calls for this stage.
        void Run(Ref<Render::Encoder> Encoder) override;

    private:

        /// \brief Enumerates the different rendering techniques available in the debug stage.
        enum class Kind : UInt8
        {
            Grid,       ///< Technique for rendering the infinite tile grid.
            Boundary,   ///< Technique for rendering the bounding box of an entity.
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

        /// \brief Draws the infinite tile grid that covers the whole viewport.
        ///
        /// \param Encoder The render encoder used to submit draw calls.
        void DrawGrid(Ref<Render::Encoder> Encoder);

        /// \brief Draws the bounding box of every entity that survives frustum culling, as a single batch.
        ///
        /// \param Encoder The render encoder used to submit draw calls.
        void DrawBoundaries(Ref<Render::Encoder> Encoder);

        /// \brief Represents the per-pass data for the grid technique.
        struct GpuGridLayout final
        {
            /// The inverse view-projection, which recovers the world position of every pixel.
            Matrix4x4 Camera;

            /// The size of a region in tiles, which sets the period of the accented lines.
            Vector2   Dimension;
        };

        /// \brief Represents the per-instance data for the bounding box of an entity.
        struct GpuBoundaryLayout final
        {
            /// The center of the bounding box in world space.
            Vector2 Center;

            /// The half-size of the bounding box in world space.
            Vector2 Extent;
        };

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Techniques                  mTechniques;
        Sequence<GpuBoundaryLayout> mBoundaryData;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        ConstPtr<Director>          mDirector;
        UInt8                       mProperties;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Scene::Query                mQrDrawBoundaries;
    };
}
