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

#include "Tileon.Render/Scribe.hpp"
#include "Tileon.Render/Component/Mosaic.hpp"
#include "Tileon.Render/Director.hpp"
#include "Tileon.Render/Tileset.hpp"
#include <Zyphryon.Render/Pass.hpp>
#include <Zyphryon.Scene/Service.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Pipeline
{
    /// \brief Represents the geometry stage of the rendering pipeline, responsible for rendering objects.
    class Geometry final : public Render::Pass, public Engine::Locator<Content::Service>
    {
    public:

        /// \brief Constructs the stage instance with the specified service host.
        ///
        /// \param Host    The service host to associate with the stage.
        /// \param Tileset The tileset containing the tile data to be used for rendering the stage.
        Geometry(Ref<Engine::Subsystem::Host> Host, ConstRef<Tileset> Tileset);

        /// \brief Sets the director the stage resolves its draws against for the current frame.
        ///
        /// \param Director The director instance providing camera and view management for rendering.
        ZY_INLINE void SetDirector(ConstRef<Director> Director)
        {
            mDirector = AddressOf(Director);
        }

        /// \brief Sets how many pixels of art the project maps onto one world unit.
        ///
        /// \param Density The pixel density sprites are measured against.
        ZY_INLINE void SetDensity(UInt16 Density)
        {
            mDensity = Density;
        }

        /// \brief Executes the stage's main logic.
        ///
        /// \param Encoder The render encoder used to submit draw calls for this stage.
        void Run(Ref<Render::Encoder> Encoder) override;

    private:

        /// \brief Enumerates the different rendering techniques available in the geometry stage.
        ///
        /// \remark Each value names the technique file the stage loads it from.
        enum class Kind : UInt8
        {
            Sprite_Opaque,         ///< Technique for rendering opaque sprites without normal mapping.
            Sprite_Opaque_Lit,     ///< Technique for rendering opaque sprites with normal mapping.
            Sprite_Transparent,    ///< Technique for rendering transparent sprites without normal mapping.
            Sprite_Transparent_Lit,///< Technique for rendering transparent sprites with normal mapping.
            Tile_Opaque,           ///< Technique for rendering the base layer, whose art covers the ground whole.
            Tile_Masked,           ///< Technique for rendering the layers above the base, whose art cuts out.
            Text,                  ///< Technique for rendering text glyphs from a font atlas.
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

        /// \brief Draws one layer of a region within the specified boundaries.
        ///
        /// \param Region     The region to draw.
        /// \param Mosaic     The region's block cache, rebuilt in place when it has gone stale.
        /// \param Origin     The origin point in world coordinates where the region should be drawn.
        /// \param Boundaries The boundaries within which to draw the region.
        /// \param Layer      The layer of the region to draw.
        void DrawRegion(ConstRef<Region> Region, Ref<Mosaic> Mosaic, IntVector3 Origin, IntRect Boundaries, Tile::Layer Layer);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Scribe             mScribe;
        Techniques         mTechniques;
        ConstRef<Tileset>  mTileset;
        ConstPtr<Director> mDirector;
        UInt16             mDensity;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Scene::Query       mQrDrawOpaqueUnlitSprites;
        Scene::Query       mQrDrawOpaqueLitSprites;
        Scene::Query       mQrDrawTransparentUnlitSprites;
        Scene::Query       mQrDrawTransparentLitSprites;
        Scene::Query       mQrDrawTexts;
        Scene::Query       mQrDrawRegions;
    };
}