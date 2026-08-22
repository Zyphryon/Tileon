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

#include "Types.hpp"
#include "Tileon.Render/Component.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Paints the terrain the ground blends between.
    class Ground final
    {
    public:

        /// \brief The widest the ground brush reaches from the unit under the cursor.
        static constexpr UInt8  kMaxSize = 16;

        /// \brief How long the ground brush waits before it lays another pass down on the same spot.
        static constexpr Real64 kCadence = 1.0 / 20.0;

    public:

        /// \brief Constructs a ground brush painting into the specified context's world.
        ///
        /// \param Context The reference to the context that the brush will interact with.
        Ground(Ref<Context> Context);

        /// \brief Sets the footprint the brush covers.
        ///
        /// \param Shape The shape to set.
        ZY_INLINE void SetShape(Shape Shape)
        {
            mShape = Shape;
        }

        /// \brief Gets the footprint the brush covers.
        ///
        /// \return The current shape.
        ZY_INLINE Shape GetShape() const
        {
            return mShape;
        }

        /// \brief Sets how far the brush reaches from the unit under the cursor.
        ///
        /// \param Size The reach to set, in world units, from one to \ref kMaxSize.
        ZY_INLINE void SetSize(UInt8 Size)
        {
            mSize = Clamp(Size, UInt8(1), kMaxSize);
        }

        /// \brief Gets how far the brush reaches from the unit under the cursor.
        ///
        /// \return The current reach, in world units.
        ZY_INLINE UInt8 GetSize() const
        {
            return mSize;
        }

        /// \brief Sets how much of the brush lands on each pass.
        ///
        /// \param Flow The strength to set.
        ZY_INLINE void SetFlow(UInt8 Flow)
        {
            mFlow = Flow;
        }

        /// \brief Gets how much of the brush lands on each pass.
        ///
        /// \return The current strength.
        ZY_INLINE UInt8 GetFlow() const
        {
            return mFlow;
        }

        /// \brief Sets how the brush gives way towards its rim.
        ///
        /// \param Falloff The falloff to set.
        ZY_INLINE void SetFalloff(Falloff Falloff)
        {
            mFalloff = Falloff;
        }

        /// \brief Gets how the brush gives way towards its rim.
        ///
        /// \return The current falloff.
        ZY_INLINE Falloff GetFalloff() const
        {
            return mFalloff;
        }

        /// \brief Gets how much of the brush lands on one unit of the ground.
        ///
        /// \param Unit   The unit being covered, in world units.
        /// \param Offset The unit's offset from the centre of the stroke, in world units.
        /// \return The share of the brush that lands on the unit.
        UInt8 Cover(IntVector2 Unit, IntVector2 Offset) const;

        /// \brief Paints the ground with a slice at the specified placement in the world.
        ///
        /// \param Brush     The brush the stroke is laid down with.
        /// \param Command   The command to execute, where removing paints the region's first slot back.
        /// \param Placement The placement in the world the paint lands on.
        /// \param Object    The slice of the shared array to paint.
        void Execute(Brush Brush, Command Command, Placement Placement, UInt32 Object);

        /// \brief Lays down every stroke that was waiting on a region the world had yet to bring in.
        void Flush();

    private:

        /// \brief Holds a stroke that fell on a region the world was still bringing in.
        struct Deferred final
        {
            /// The x-coordinate of the region the stroke was waiting on.
            SInt16     RegionX;

            /// The y-coordinate of the region the stroke was waiting on.
            SInt16     RegionY;

            /// The brush the stroke was laid down with.
            Brush      Brush;

            /// Whether the stroke lays a terrain down or takes it away.
            Command    Command;

            /// The area the stroke covers, in world units.
            IntRect    Area;

            /// The point the brush is shaped around, in world units.
            IntVector2 Centre;

            /// The slice the stroke paints.
            UInt16     Slice;
        };

        /// \brief Paints one region's share of an area of ground.
        ///
        /// \param Actor   The region's actor, captured so the stroke can be taken back.
        /// \param Surface The surface to paint.
        /// \param Brush   The brush the stroke is laid down with.
        /// \param Command The command to execute, where removing paints the region's founding slice back.
        /// \param Area    The area to paint, in world unit coordinates.
        /// \param Centre  The unit the brush is shaped around, in world unit coordinates.
        /// \param Slice   The slice of the shared array to paint.
        void Apply(
            Scene::Entity Actor,
            Ptr<Splatmap> Surface,
            Brush         Brush,
            Command       Command,
            IntRect       Area,
            IntVector2    Centre,
            UInt16        Slice);

        /// \brief Pulls one point of the ground towards the average of the ones around it.
        ///
        /// \param Surface  The surface to soften.
        /// \param X        The x-coordinate of the point within the region.
        /// \param Y        The y-coordinate of the point within the region.
        /// \param Strength How far the point is carried towards its neighbours.
        void Soften(Ptr<Splatmap> Surface, UInt8 X, UInt8 Y, UInt8 Strength);

        /// \brief Takes the terrain under a placement as the one the palette paints with.
        ///
        /// \param Placement The placement in the world to read.
        void Pick(Placement Placement);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Context>       mContext;
        Shape              mShape;
        UInt8              mSize;
        UInt8              mFlow;
        Falloff            mFalloff;
        Sequence<Deferred> mDeferred;
    };
}