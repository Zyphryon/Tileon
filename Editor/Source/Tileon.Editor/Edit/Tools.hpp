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

#include "Entities.hpp"
#include "Ground.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Routes an editing command to the ground or the entities, whichever the mode names.
    class Tools final
    {
    public:

        /// \brief The commands either side of the toolbox takes.
        using Command = Editor::Command;

        /// \brief The brushes the toolbox is held with.
        using Brush   = Editor::Brush;

        /// \brief The sides of the toolbox a command may be routed to.
        using Mode    = Editor::Mode;

        /// \brief The footprints the ground brush covers.
        using Shape   = Editor::Shape;

        /// \brief The ways the ground brush gives way at its rim.
        using Falloff = Editor::Falloff;

    public:

        /// \brief Constructs a toolbox with the specified context reference.
        ///
        /// \param Context The reference to the context that the toolbox will interact with.
        Tools(Ref<Context> Context);

        /// \brief Gets the brush that paints the terrain the ground blends between.
        ///
        /// \return The ground brush.
        ZY_INLINE Ref<Ground> GetGround()
        {
            return mGround;
        }

        /// \brief Gets the tool that places and picks the entities on top of the ground.
        ///
        /// \return The entity tool.
        ZY_INLINE Ref<Entities> GetEntities()
        {
            return mEntities;
        }

        /// \brief Sets the current editing mode.
        ///
        /// \param Mode The mode to set.
        ZY_INLINE void SetMode(Mode Mode)
        {
            mContext.SetEnum(Session::kToolsMode, Mode);
        }

        /// \brief Gets the current editing mode.
        ///
        /// \return The current editing mode.
        ZY_INLINE Mode GetMode() const
        {
            return mContext.GetEnum(Session::kToolsMode, Mode::Ground);
        }

        /// \brief Sets the brush subsequent commands are issued with.
        ///
        /// \param Brush The brush to set.
        ZY_INLINE void SetBrush(Brush Brush)
        {
            mBrush = Brush;
        }

        /// \brief Gets the brush subsequent commands are issued with.
        ///
        /// \return The current brush.
        ZY_INLINE Brush GetBrush() const
        {
            return mBrush;
        }

        /// \brief Sets whether placement snaps to whole units.
        ///
        /// \param Aligned Whether placement is aligned.
        ZY_INLINE void SetAligned(Bool Aligned)
        {
            mAligned = Aligned;
        }

        /// \brief Gets whether placement snaps to whole units.
        ///
        /// \return `true` when placement is aligned, `false` otherwise.
        ZY_INLINE Bool IsAligned() const
        {
            return mAligned;
        }

        /// \brief Executes an editing command at the specified placement in the world.
        ///
        /// \param Command   The editing command to execute (e.g., add or remove).
        /// \param Placement The placement in the world where the command should be executed.
        /// \param Object    The unique identifier for the object to be added or removed.
        void Execute(Command Command, Placement Placement, UInt32 Object);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Context> mContext;
        Ground       mGround;
        Entities     mEntities;
        Brush        mBrush;
        Bool         mAligned;
    };
}