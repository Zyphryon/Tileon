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

#include "Tileon.Render/Sprite/Appearance.hpp"
#include "Zyphryon.Render/Collector.hpp"
#include "Zyphryon.Render/Encoder.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Batch
{
    /// \brief Records the sprites of a pass and writes back the batches the collector hands it.
    class Sprite final
    {
    public:

        /// \brief Constructs a sprite batcher allocating from the specified service.
        ///
        /// \param Service   The service the transient instance streams are allocated from.
        /// \param Collector The collector the sprites are ordered and batched by.
        /// \param Kind      The tag every draw is stamped with, so the drain routes its batches back here.
        Sprite(ConstRetainer<Graphic::Service> Service, Ref<Render::Collector> Collector, UInt32 Kind);

        /// \brief Sets the technique subsequent sprites are recorded under.
        ///
        /// \param Technique The technique to set for subsequent sprites.
        /// \param Variant   The features the caller turns on itself, beyond the ones a material implies.
        ZY_INLINE void SetTechnique(ConstRetainer<Graphic::Technique> Technique, Graphic::Technique::Key Variant)
        {
            mTechnique = Technique;
            mVariant   = Variant;
        }

        /// \brief Sets the priority subsequent sprites are recorded under.
        ///
        /// \param Priority The queue the draws are collected into, which the open pass decides.
        ZY_INLINE void SetPriority(Render::Collector::Priority Priority)
        {
            mPriority  = Priority;
        }

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

        /// \brief Represents the per-instance data for a sprite.
        struct Layout final
        {
            /// The transformation matrix to apply to the sprite for positioning, scaling, and rotation.
            Array<Real32, 12> Transform;

            /// The source rectangle within the sprite's texture, defining the portion of the texture to use.
            Rect              Frame;

            /// The size the sprite covers, in world units, before the transform is applied.
            Vector2           Size;

            /// The tint applied to the sprite, as an 8-bit integer color (RGBA).
            IntColor8         Color;

            /// The bits the technique tests to mirror and transpose the art as it is laid down.
            UInt32            Facing;
        };

        /// \brief Represents a draw recorded for a sprite, with everything its batch is keyed by.
        struct Command final
        {
            /// The graphics technique to use for rendering the sprite.
            ConstPtr<Graphic::Technique> Technique;

            /// The features the technique is drawn with, which keeps one variant's draws out of another's batch.
            Graphic::Technique::Key      Variant;

            /// The material to use for rendering the sprite, containing shader parameters and resources.
            ConstPtr<Graphic::Material>  Material;

            /// The input data for the sprite.
            Layout                       Layout;
        };

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Retainer<Graphic::Service>   mService;
        Ref<Render::Collector>       mCollector;
        UInt32                       mKind;
        Retainer<Graphic::Technique> mTechnique;
        Graphic::Technique::Key      mVariant;
        Render::Collector::Priority  mPriority;
        Sequence<Command>            mCommands;
    };
}