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

#include "Lens.hpp"
#include <Zyphryon.Base/Container/Bag.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Manipulates the selected entity's pose directly in the viewport.
    class Gizmo final
    {
    public:

        /// \brief Defines what the handles manipulate.
        enum class Mode : UInt8
        {
            Move,   ///< Translates the entity across the ground.
            Rotate, ///< Turns the entity about its own origin.
            Scale,  ///< Resizes the entity about its own origin.
        };

    public:

        /// \brief Constructs a gizmo backed by the specified context.
        ///
        /// \param Context The context supplying the director the handles are projected through.
        explicit Gizmo(Ref<Context> Context);

        /// \brief Sets what the handles manipulate.
        ///
        /// \param Mode The mode to switch to.
        ZY_INLINE void SetMode(Mode Mode)
        {
            mMode = Mode;
        }

        /// \brief Gets what the handles manipulate.
        ///
        /// \return The current mode.
        ZY_INLINE Mode GetMode() const
        {
            return mMode;
        }

        /// \brief Draws the handles over the viewport and applies whatever the user drags.
        ///
        /// The handles sit on the primary entity's origin, which is the pivot the whole selection turns and
        /// scales about; a drag transforms every selected entity together.
        ///
        /// \param Composer  The UI composer used to render the handles.
        /// \param Selection The set of selected entities (instance-root ids) the drag transforms together.
        /// \param Primary   The entity the handles anchor to, which is the pivot for the group.
        /// \param Origin    The top-left corner of the viewport, in screen space.
        /// \param Size      The size of the viewport, in screen space.
        /// \return `true` while the gizmo owns the cursor, which is when the viewport must not act on it.
        Bool Draw(Ref<UI::Composer> Composer, ConstRef<Bag<UInt64>> Selection, Scene::Entity Primary, ImVec2 Origin, ImVec2 Size);

    private:

        static constexpr UInt32 kColorAxisX  = IM_COL32(220,  70,  70, 255);
        static constexpr UInt32 kColorAxisY  = IM_COL32( 90, 200,  90, 255);
        static constexpr UInt32 kColorPlane  = IM_COL32( 90, 160, 235, 255);
        static constexpr UInt32 kColorActive = IM_COL32(255, 205,  70, 255);

        /// \brief Identifies which handle a drag belongs to.
        enum class Handle : UInt8
        {
            None,    ///< No handle is engaged.
            AxisX,   ///< The handle constrained to the world's x axis.
            AxisY,   ///< The handle constrained to the world's y axis.
            Plane,   ///< The handle that acts on both axes independently.
            Uniform, ///< The handle that acts on both axes by the same factor.
        };

        /// \brief How wide the band around the rotation ring is that still grabs it, in screen pixels.
        static constexpr Real32 kRingBand   = 12.0f;

        /// \brief The length of an axis handle, in screen pixels.
        static constexpr Real32 kAxisLength = 64.0f;

        /// \brief How close the cursor must come to a handle to engage it, in screen pixels.
        static constexpr Real32 kPickRadius = 7.0f;

        /// \brief The radius of the rotation ring, in screen pixels.
        static constexpr Real32 kRingRadius = 52.0f;

        /// \brief The captured start state of one selected entity, taken when a drag begins.
        struct Snapshot final
        {
            /// The entity this snapshot belongs to.
            Scene::Entity Actor;

            /// The entity's absolute origin at grab time, used as the orbit radius for rotate and scale.
            Placement     Start;

            /// The entity's pose at grab time, the basis every transform is applied relative to.
            Pose          Origin;
        };

        /// \brief Captures the start state of every selected entity so a drag can transform them as a group.
        ///
        /// \param Selection The selected entity ids.
        /// \param Primary   The pivot entity, always included even when the set is empty.
        void CaptureSnapshots(ConstRef<Bag<UInt64>> Selection, Scene::Entity Primary);

        /// \brief Gets an entity's origin, which is the point everything here manipulates it about.
        ///
        /// This reads the pose rather than the transform, because the two disagree the moment an anchor exists:
        /// the transform resolves to where the sprite's corner lands, whereas the pose places the anchor itself,
        /// and that is both the entity's origin and the point it turns and scales about.
        ///
        /// \param Actor The entity to locate.
        /// \return The entity's placement, or a default placement when it sits outside a region.
        Placement GetOrigin(Scene::Entity Actor) const;

        /// \brief Gets the region that currently holds an entity.
        ///
        /// \param Actor The entity to look up.
        /// \return The region component holding the entity, or `nullptr` when it sits outside one.
        ConstPtr<Region> GetRegion(Scene::Entity Actor) const;

        /// \brief Draws the handles that translate the entity, and reports which one the cursor wants.
        ///
        /// \param Composer The UI composer used to render the handles.
        /// \param Anchor   The entity's position, in screen space.
        /// \return The handle under the cursor, or `Handle::None`.
        Handle DrawMove(Ref<UI::Composer> Composer, ImVec2 Anchor) const;

        /// \brief Draws the ring that turns the entity, and reports whether the cursor wants it.
        ///
        /// \param Composer The UI composer used to render the ring.
        /// \param Anchor   The entity's position, in screen space.
        /// \return The handle under the cursor, or `Handle::None`.
        Handle DrawRotate(Ref<UI::Composer> Composer, ImVec2 Anchor) const;

        /// \brief Draws the handles that resize the entity, and reports which one the cursor wants.
        ///
        /// \param Composer The UI composer used to render the handles.
        /// \param Anchor   The entity's position, in screen space.
        /// \return The handle under the cursor, or `Handle::None`.
        Handle DrawScale(Ref<UI::Composer> Composer, ImVec2 Anchor) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Context> mContext;
        Mode         mMode;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Handle             mHandle;
        Placement          mGrab;
        Placement          mStart;
        Pose               mOrigin;
        ImVec2             mBearing;
        Real32             mReach;
        Sequence<Snapshot> mSnapshots;
    };
}
