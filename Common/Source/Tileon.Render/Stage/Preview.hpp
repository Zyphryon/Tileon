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
#include <Zyphryon.Render/Target.hpp>
#include <Zyphryon.Scene/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Stage
{
    /// \brief Represents the preview stage, which resolves one raw buffer onto the view and annotates it.
    class Preview final : public Render::Pass, public Engine::Locator<Graphic::Service>
    {
    public:

        /// \brief Enumerates the buffers the stage can resolve, each with a technique of its own.
        enum class Kind : UInt8
        {
            Albedo,     ///< Technique for showing the scene's base color as it is composed.
            Normal,     ///< Technique for showing the surface normals as the raw bytes the buffer stores.
            Depth,      ///< Technique for showing the elevation the depth buffer unprojects to.
            Radiance,   ///< Technique for showing the accumulated lighting on its own.
        };

        /// \brief Enumerates the diagnostic overlays that can be drawn over the resolved buffer.
        enum class Property : UInt8
        {
            Boundaries = 0b00000001,    ///< Draws the world-space bounding box of every visible entity.
            Grid       = 0b00000010,    ///< Draws the infinite tile grid, accented on region borders.
        };

    public:

        /// \brief Constructs the stage instance with the specified service host.
        ///
        /// \param Host     The service host to associate with the stage.
        /// \param Albedo   The target holding the scene's base color.
        /// \param Normal   The target holding the scene's surface normals.
        /// \param Depth    The target holding the scene's depth.
        /// \param Radiance The target holding the scene's accumulated lighting.
        Preview(
            Ref<Engine::Subsystem::Host> Host,
            ConstRef<Render::Target>     Albedo,
            ConstRef<Render::Target>     Normal,
            ConstRef<Render::Target>     Depth,
            ConstRef<Render::Target>     Radiance);

        /// \brief Sets the director the stage resolves its draws against for the current frame.
        ///
        /// \param Director The director instance providing camera and view management for rendering.
        ZY_INLINE void SetDirector(ConstRef<Director> Director)
        {
            mDirector = AddressOf(Director);
        }

        /// \brief Sets the buffer the stage resolves onto the view.
        ///
        /// \param Source The buffer to resolve.
        ZY_INLINE void SetSource(Kind Source)
        {
            mSource = Source;
        }

        /// \brief Changes the state of a specific overlay for the stage.
        ///
        /// \param Mask   The overlay to set or clear.
        /// \param Enable `true` to set the overlay, `false` to clear it.
        ZY_INLINE void SetProperty(Property Mask, Bool Enable)
        {
            mProperties = SetOrClearBit(mProperties, Enum::Cast(Mask), Enable);
        }

        /// \brief Checks if the stage has a specific overlay enabled.
        ///
        /// \param Mask The overlay to check.
        /// \return `true` if the overlay is enabled, `false` otherwise.
        ZY_INLINE Bool HasProperty(Property Mask) const
        {
            return HasBit(mProperties, Enum::Cast(Mask));
        }

        /// \brief Executes the stage's main logic.
        ///
        /// \param Encoder The render encoder used to submit draw calls for this stage.
        void Run(Ref<Render::Encoder> Encoder) override;

    private:

        /// \brief Enumerates the overlay techniques the stage draws over the resolved buffer.
        enum class Overlay : UInt8
        {
            Grid,           ///< Technique for rendering the infinite tile grid.
            Boundary,       ///< Technique for rendering the bounding box of an entity, flat when the view is aligned.
        };

        /// \brief Defines a type alias for the techniques that annotate one, indexed by \ref Overlay.
        using Overlays   = Array<Retainer<Graphic::Technique>, Enum::Count<Overlay>()>;

        /// \brief Defines a type alias for the buffers the stage can resolve, indexed by \ref Kind.
        using Sources    = Array<ConstPtr<Render::Target>, Enum::Count<Kind>()>;

        /// \brief Represents the per-pass data for the grid technique.
        struct GpuGridLayout final
        {
            /// The size of a region in tiles, which sets the period of the accented lines.
            Vector2 Dimension;
        };

        /// \brief Represents the per-instance data for the bounding box of an entity.
        struct GpuBoundaryLayout final
        {
            /// The center of the bounding box in world space, relative to the camera's region.
            Vector3 Center;

            /// The half-size of the bounding box in world space.
            Vector3 Extent;
        };

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

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Retainer<Graphic::Technique> mTechnique;
        Overlays                     mOverlays;
        Sources                      mSources;
        Sequence<GpuBoundaryLayout>  mBoundaries;
        ConstPtr<Director>           mDirector;
        Kind                         mSource;
        UInt8                        mProperties;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Scene::Query                 mQrDrawGeometryBoundaries;
        Scene::Query                 mQrDrawLightingBoundaries;
    };
}