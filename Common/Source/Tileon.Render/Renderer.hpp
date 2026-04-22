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

#include "Component.hpp"
#include "Tileon.World/Region.hpp"
#include <Zyphryon.Render/Renderer/Canvas.hpp>
#include <Zyphryon.Scene/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents the renderer responsible for executing the rendering pipeline.
    class Renderer final : public Locator<Content::Service, Scene::Service>
    {
    public:

        /// \brief Properties that can be toggled for the renderer.
        enum class Property : UInt8
        {
            DrawGuide   = 0b00000001,   ///< Draws guide lines for debugging purposes.
            DrawVolumes = 0b00000010,   ///< Draws collision volumes for debugging purposes.
        };

    public:

        /// \brief Constructs a renderer with the specified service host.
        ///
        /// \param Host The service host to associate with the renderer.
        Renderer(Ref<Service::Host> Host);

        /// \brief Sets or clears a specific property for the renderer.
        ///
        /// \param Mask   The property to set or clear.
        /// \param Enable `true` to set the property, `false` to clear it
        void SetProperty(Property Mask, Bool Enable);

        /// \brief Checks if a specific property is enabled for the renderer.
        ///
        /// \param Mask The property to check.
        /// \return `true` if the property is enabled, `false` otherwise.
        ZYPHRYON_INLINE Bool HasProperty(Property Mask) const
        {
            return HasBit(mProperties, Enum::Cast(Mask));
        }

        /// \brief Presents the rendered scene to the display, executing the rendering pipeline.
        ///
        /// \param Projection The projection matrix to use for rendering.
        /// \param Frustum    The frustum rectangle defining the visible area for rendering.
        /// \param Origin     The origin point for rendering, typically representing the camera's position in world space.
        void Present(ConstRef<Matrix4x4> Projection, IntRect Frustum, IntVector2 Origin);

        /// \brief Gets a material resource based on the provided path, with a fallback to a default material.
        ///
        /// \param Path The path to the material resource to retrieve.
        /// \return The material resource corresponding to the provided path, or a default material if fails.
        ZYPHRYON_INLINE Tracker<Graphic::Material> GetMaterial(ConstRef<Content::Uri> Path)
        {
            Tracker<Graphic::Material> Material = nullptr;

            if (Path.IsValid())
            {
                Material = GetService<Content::Service>().Load<Graphic::Material>(Path);
            }

            if (Material == nullptr || Material->HasFailed())
            {
                constexpr ConstStr8 Default = "Resources://Material/Default.material";
                Material = GetService<Content::Service>().Load<Graphic::Material>(Default);
            }
            return Material;
        }

    private:

        /// \brief Loads necessary resources and initializes the renderer when the content service is loaded.
        ///
        /// \param Content The content service to load resources from.
        void OnLoad(Ref<Content::Service> Content);

        /// \brief Registers the renderer with the specified scene service.
        ///
        /// \param Scene The scene service to register with.
        void OnRegister(Ref<Scene::Service> Scene);

        /// \brief Draws a specific region of the world.
        ///
        /// \param Region     The region of the world to draw.
        /// \param Palette    The palette to use for drawing the region.
        /// \param Boundaries The boundaries within which to draw the region, typically representing the visible.
        void DrawRegion(ConstRef<Region> Region, Ref<Palette> Palette, IntRect Boundaries);

        /// \brief Draws guide lines for debugging purposes within the specified boundaries.
        void DrawGuide();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Render::Canvas  mRenderer;
        Scene::Pipeline mPipeline;
        UInt32          mProperties;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        IntRect         mFrustum;
        IntVector2      mOrigin;
    };
}

