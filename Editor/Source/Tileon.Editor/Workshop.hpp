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

#include "Tileon.Runtime/Controller.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Provides tools and functionality for painting and editing the game world within the editor.
    class Workshop final
    {
    public:

        /// \brief Defines the different commands that can be executed.
        enum class Command : UInt8
        {
            Add,        ///< A command used for adding tiles or objects to the scene.
            Remove,     ///< A command used for removing tiles or objects from the scene.
        };

        /// \brief Defines the different brush types that can be used for editing the scene.
        enum class Brush : UInt8
        {
            Hand,       ///< A brush used for panning the view.
            Select,     ///< A brush used for selecting objects or areas in the scene.
            Pencil,     ///< A brush used for painting individual tiles.
            Bucket,     ///< A brush used for filling an area with a specific tile type.
        };

        /// \brief Defines the different levels that can be painted on.
        enum class Level : UInt8
        {
            Base,   ///< The floor level, typically representing the base layer of the scene.
            Detail, /// The detail level, used for painting additional details on top of the base layer.
        };

        /// \brief Defines the different modes that can be used for editing the scene.
        enum class Mode : UInt8
        {
            Tile,       ///< A mode used for painting tiles in the scene.
            Entity,     ///< A mode used for painting entities (objects) in the scene.
        };

    public:

        /// \brief Constructs a workshop with the specified controller reference.
        ///
        /// \param Controller The reference to the controller that the workshop will interact with.
        Workshop(Ref<Controller> Controller);

        /// \brief Sets the current editing mode.
        ///
        /// \param Mode The mode to set.
        ZYPHRYON_INLINE void SetMode(Mode Mode)
        {
            mMode = Mode;
        }

        /// \brief Gets the current editing mode.
        ///
        /// \return The current editing mode.
        ZYPHRYON_INLINE Mode GetMode() const
        {
            return mMode;
        }

        /// \brief Sets the current level.
        ///
        /// \param Level The level to set.
        ZYPHRYON_INLINE void SetLevel(Level Level)
        {
            mLevel = Level;
        }

        /// \brief Gets the current level.
        ///
        /// \return The current level.
        ZYPHRYON_INLINE Level GetLevel() const
        {
            return mLevel;
        }

        /// \brief Sets the current brush type.
        ///
        /// \param Brush The brush type to set.
        ZYPHRYON_INLINE void SetBrush(Brush Brush)
        {
            mBrush = Brush;
        }

        /// \brief Gets the current brush type.
        ///
        /// \return The current brush type.
        ZYPHRYON_INLINE Brush GetBrush() const
        {
            return mBrush;
        }

        /// \brief Executes a editing command at the specified placement in the world.
        ///
        /// \param Command   The editing command to execute (e.g., add or remove).
        /// \param Placement The placement in the world where the command should be executed,.
        void Execute(Command Command, Placement Placement);

    private:

        /// \brief Executes a tile editing command at the specified placement in the world.
        ///
        /// \param Command   The tile editing command to execute (e.g., add or remove).
        /// \param Placement The placement in the world where the tile command should be executed.
        void ExecuteOnTiles(Command Command, Placement Placement);

        /// \brief Executes an entity editing command at the specified placement in the world.
        ///
        /// \param Command   The entity editing command to execute (e.g., add or remove).
        /// \param Placement The placement in the world where the entity command should be executed.
        void ExecuteOnEntities(Command Command, Placement Placement);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Controller> mController;
        Mode            mMode;
        Level           mLevel;
        Brush           mBrush;
    };
}