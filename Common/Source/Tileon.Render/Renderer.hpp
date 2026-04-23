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
#include "Tileset.hpp"
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

        /// \brief Loads necessary resources and initializes the renderer.
        void Load();

        /// \brief Saves any necessary data or state related to the renderer.
        void Save();

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

        /// \brief Gets the tileset associated with the renderer, which manages the rendering data for different terrains.
        ///
        /// \return The tileset associated with the renderer.
        ZYPHRYON_INLINE Ref<Tileset> GetTileset()
        {
            return mTileset;
        }

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

        /// \brief Draws a region within the specified boundaries.
        ///
        /// \param Region     The region to draw.
        /// \param Boundaries The boundaries within which to draw the region.
        void DrawRegion(ConstRef<Region> Region, IntRect Boundaries);

        /// \brief Draws a tile at the specified position.
        ///
        /// \param Position The position to draw the tile at.
        /// \param Span     The span of the tile in terms of tile columns and rows.
        /// \param Weight   The weight of the tile, which may affect its visual representation.
        /// \param Visual   The visual representation of the tile, typically containing material and animation data.
        void DrawTile(Vector3 Position, IntVector2 Span, UInt8 Weight, ConstRef<Tileset::Entry> Visual);

        /// \brief Draws guide lines for debugging purposes within the specified boundaries.
        void DrawGuide();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Render::Canvas  mRenderer;
        Scene::Pipeline mPipeline;
        UInt32          mProperties;
        Tileset         mTileset;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        IntRect         mFrustum;
        IntVector2      mOrigin;
    };
}

