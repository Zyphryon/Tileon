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

#include "Common.hpp"
#include "Tileon.Render/Component/Appearance.hpp"
#include "Zyphryon.Render/Encoder.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Records the sprites of a pass and writes back the batches the collector hands it.
    class Sprites final
    {
    public:

        /// \brief Constructs a sprite batcher allocating from the specified service.
        ///
        /// \param Service   The service the transient instance streams are allocated from.
        /// \param Collector The collector the sprites are ordered and batched by.
        Sprites(ConstRetainer<Graphic::Service> Service, Ref<Render::Collector> Collector);

        /// \brief Sets the technique subsequent sprites are recorded under.
        ///
        /// \param Technique The technique to set for subsequent sprites.
        void SetTechnique(ConstRetainer<Graphic::Technique> Technique);

        /// \brief Records a sprite with the specified parameters.
        ///
        /// \param Appearance The appearance supplying the material the sprite samples and the region it shows.
        /// \param Size       The size the sprite covers, in world units, before the transform is applied.
        /// \param Transform  The transformation matrix to apply to the sprite for positioning, scaling, and rotation.
        /// \param Tint       The tint color to apply to the sprite.
        void Draw(ConstRef<Appearance> Appearance, Vector2 Size, ConstRef<Matrix4x3> Transform, IntColor8 Tint);

        /// \brief Drops the sprites recorded so far.
        void Reset();

        /// \brief Writes one batch of sprites as a single instanced command through the encoder.
        ///
        /// \param Encoder  The encoder that builds the resulting draw command.
        /// \param Commands The span of draw calls forming the batch.
        void Write(Ref<Render::Encoder> Encoder, ConstSpan<Render::Collector::Command> Commands);

    private:

        /// \brief Defines a structure representing the input data for drawing a sprite in the GPU.
        struct SpriteLayout final
        {
            /// The transformation matrix to apply to the sprite for positioning, scaling, and rotation.
            Matrix4x3Packed Transform;

            /// The source rectangle within the sprite's texture, defining the portion of the texture to use.
            Rect            Frame;

            /// Additional data for the sprite, such as size stored as a 2D vector.
            Vector2         Size;

            /// Color tint to apply to the sprite, represented as an 8-bit integer color (RGBA).
            IntColor8       Color;
        };

        /// \brief Defines a draw command for a sprite, containing its input data.
        struct SpriteCommand final
        {
            /// The graphics technique to use for rendering the sprite.
            ConstPtr<Graphic::Technique> Technique;

            /// The material to use for rendering the sprite, containing shader parameters and resources.
            ConstPtr<Graphic::Material>  Material;

            /// The input data for the sprite.
            SpriteLayout                 Layout;
        };

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Retainer<Graphic::Service>   mService;
        Ref<Render::Collector>       mCollector;
        Retainer<Graphic::Technique> mTechnique;
        Render::Collector::Priority  mPriority;
        Graphic::Object              mPipeline;
        Sequence<SpriteCommand>      mSprites;
    };
}