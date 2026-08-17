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

#include "Batch.hpp"
#include "Tileon.Render/Component/Appearance.hpp"
#include "Zyphryon.Render/Encoder.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Batcher
{
    /// \brief Records the sprites of a pass and writes back the batches the collector hands it.
    class Sprite final
    {
    public:

        /// \brief Constructs a sprite batcher allocating from the specified service.
        ///
        /// \param Service   The service the transient instance streams are allocated from.
        /// \param Collector The collector the sprites are ordered and batched by.
        Sprite(ConstRetainer<Graphic::Service> Service, Ref<Render::Collector> Collector);

        /// \brief Sets the technique subsequent sprites are recorded under.
        ///
        /// \param Technique The technique to set for subsequent sprites.
        /// \param Variant  The features the caller turns on itself, beyond the ones a material implies.
        /// \param Priority The queue the draws are collected into, which the open pass decides.
        void SetTechnique(ConstRetainer<Graphic::Technique> Technique,
            Graphic::Technique::Key Variant, Render::Collector::Priority Priority);

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
        struct Layout final
        {
            /// The transformation matrix to apply to the sprite for positioning, scaling, and rotation.
            Array<Real32, 12> Transform;

            /// The source rectangle within the sprite's texture, defining the portion of the texture to use.
            Rect              Frame;

            /// Additional data for the sprite, such as size stored as a 2D vector.
            Vector2           Size;

            /// Color tint to apply to the sprite, represented as an 8-bit integer color (RGBA).
            IntColor8         Color;

            /// How the art is laid down, as the bits the technique tests for mirroring and transposing.
            UInt32            Facing;
        };

        /// \brief Defines a draw command for a sprite, containing its input data.
        struct Command final
        {
            /// The graphics technique to use for rendering the sprite.
            ConstPtr<Graphic::Technique> Technique;

            /// The features the technique is drawn with, which keeps one variant's draws out of another's batch.
            Graphic::Technique::Key      Variant;

            /// The material to use for rendering the sprite, containing shader parameters and resources.
            ConstPtr<Graphic::Material>  Material;

            /// The input data for the sprite.
            Batcher::Sprite::Layout      Layout;
        };

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Retainer<Graphic::Service>   mService;
        Ref<Render::Collector>       mCollector;
        Retainer<Graphic::Technique> mTechnique;
        Graphic::Technique::Key      mVariant;
        Render::Collector::Priority  mPriority;
        Sequence<Command>            mCommands;
    };
}