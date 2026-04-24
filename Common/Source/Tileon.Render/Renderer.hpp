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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents the renderer responsible for executing the rendering pipeline.
    class Renderer final : public Locator<Content::Service, Scene::Service>
    {
    public:

        /// \brief Options that can be toggled for the renderer.
        enum class Option : UInt8
        {
            DrawGuide,   ///< Draws guide lines for debugging purposes.
            DrawVolumes, ///< Draws collision volumes for debugging purposes.
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

        /// \brief Executes the rendering pipeline to compose the scene.
        ///
        /// \param Projection The projection matrix to use for rendering the scene.
        /// \param Frustum    The frustum rectangle defining the visible area of the scene.
        /// \param Origin     The origin point for rendering, representing the camera's position in world space.
        void Execute(ConstRef<Matrix4x4> Projection, IntRect Frustum, IntVector2 Origin);

        /// \brief Enables or disables a specific rendering option.
        ///
        /// \param Option  The rendering option to enable or disable.
        /// \param Enabled `true` to enable the option, `false` to disable it
        void SetOption(Option Option, Bool Enabled);

        /// \brief Checks if a specific rendering option is currently enabled.
        ///
        /// \param Option The rendering option to check.
        /// \return `true` if the option is enabled, `false` otherwise.
        Bool HasOption(Option Option) const;

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

        static constexpr Symbol kDrawGuideSystem  = "Renderer::DrawGuide";
        static constexpr Symbol kDrawVolumeSystem = "Renderer::DrawVolumes";

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

        Render::Canvas  mCanvas;
        Scene::Pipeline mPipeline;
        Tileset         mTileset;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        IntRect         mFrustum;
        IntVector2      mOrigin;
    };
}