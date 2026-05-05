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

#include "Tileon.Visual/Director.hpp"
#include "Tileon.Visual/Tileset.hpp"
#include <Zyphryon.Render/Renderer/Canvas.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Stage
{
    /// \brief Represents the geometry stage of the rendering pipeline, responsible for rendering objects.
    class Geometry final : public Locator<Content::Service>
    {
    public:

        /// \brief Constructs the stage instance with the specified service host.
        ///
        /// \param Host The service host to associate with the stage.
        Geometry(Ref<Service::Host> Host);

        /// \brief Executes the stage's main logic.
        ///
        /// \param Director The director instance providing camera and view management for rendering.
        /// \param Tileset  The tileset containing the tile data to be used for rendering the stage.
        void Run(ConstRef<Director> Director, ConstRef<Tileset> Tileset);

    private:

        /// \brief Enumerates the different rendering techniques available in the geometry stage.
        enum class Technique
        {
            SpriteOpaque,               ///< Technique for rendering opaque sprites without normal mapping.
            SpriteOpaqueWithNormal,     ///< Technique for rendering opaque sprites with normal mapping.
            SpriteNonOpaque,            ///< Technique for rendering non-opaque sprites without normal mapping.
            SpriteNonOpaqueWithNormal,  ///< Technique for rendering non-opaque sprites with normal mapping.
        };

        /// \brief Defines a type alias for a collection of pipeline trackers, one for each rendering technique.
        using Pipelines = Array<Tracker<Graphic::Pipeline>, Enum::Count<Technique>()>;

        /// \brief Registers the stage with the specified scene service.
        ///
        /// \param Scene The scene service to register with.
        void OnRegister(Ref<Scene::Service> Scene);

        /// \brief Loads the necessary resources for the stage, such as shaders and materials.
        ///
        /// \param Content The content service used to load resources for the stage.
        void OnLoad(Ref<Content::Service> Content);

        /// \brief Draws a region within the specified boundaries.
        ///
        /// \param Tileset    The tileset containing the tile data to use for drawing the region.
        /// \param Region     The region to draw.
        /// \param Origin     The origin point in world coordinates where the region should be drawn.
        /// \param Boundaries The boundaries within which to draw the region.
        void DrawRegion(ConstRef<Tileset> Tileset, ConstRef<Region> Region, IntVector2 Origin, IntRect Boundaries);

        /// \brief Draws a tile at the specified position.
        ///
        /// \param Position The position to draw the tile at.
        /// \param Span     The span of the tile in terms of tile columns and rows.
        /// \param Weight   The weight of the tile, which may affect its visual representation.
        /// \param Tile     The tile entry containing the rendering data for the tile to be drawn.
        void DrawTile(Vector3 Position, IntVector2 Span, UInt16 Weight, ConstRef<Tileset::Entry> Tile);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Render::Canvas mCanvas;         // TODO: Implement our own?
        Pipelines      mPipelines;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Scene::Query   mQrDrawSprites;
        Scene::Query   mQrDrawTexts;
        Scene::Query   mQrDrawRegions;
    };
}