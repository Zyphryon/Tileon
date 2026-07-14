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

        /// \brief Updates the workshop, typically called once per frame.
        void Tick();

        /// \brief Sets the current editing mode.
        ///
        /// \param Mode The mode to set.
        ZY_INLINE void SetMode(Mode Mode)
        {
            mMode = Mode;
        }

        /// \brief Gets the current editing mode.
        ///
        /// \return The current editing mode.
        ZY_INLINE Mode GetMode() const
        {
            return mMode;
        }

        /// \brief Sets the current level.
        ///
        /// \param Level The level to set.
        ZY_INLINE void SetLevel(Level Level)
        {
            mLevel = Level;
        }

        /// \brief Gets the current level.
        ///
        /// \return The current level.
        ZY_INLINE Level GetLevel() const
        {
            return mLevel;
        }

        /// \brief Sets the current brush type.
        ///
        /// \param Brush The brush type to set.
        ZY_INLINE void SetBrush(Brush Brush)
        {
            mBrush = Brush;
        }

        /// \brief Gets the current brush type.
        ///
        /// \return The current brush type.
        ZY_INLINE Brush GetBrush() const
        {
            return mBrush;
        }

        /// \brief Executes an editing command at the specified placement in the world.
        ///
        /// \param Command   The editing command to execute (e.g., add or remove).
        /// \param Placement The placement in the world where the command should be executed.
        /// \param Object    The unique identifier for the object to be added or removed.
        void Execute(Command Command, Placement Placement, UInt32 Object);

    private:

        /// \brief Represents a tile operation to be applied to a region.
        struct OpTile
        {
            /// The region's actor to apply the operation.
            Scene::Entity Actor;

            /// The command to apply (e.g., add or remove).
            Command       Command;

            /// The layer of the tiles to apply the command to (e.g., base or detail).
            Tile::Layer   Layer;

            /// The unique identifier for the terrain type to apply when adding tiles.
            UInt16        Terrain;

            /// The dimensions of the tile being applied.
            IntVector2    Span;

            /// The rectangular area of tiles to apply the command to, specified in tile coordinates.
            IntRect       Area;
        };

        /// \brief Executes a tile editing command at the specified placement in the world.
        ///
        /// \param Command   The tile editing command to execute (e.g., add or remove).
        /// \param Placement The placement in the world where the tile command should be executed.
        /// \param Object    The unique identifier for the object to be added or removed.
        void ExecuteOnTiles(Command Command, Placement Placement, UInt32 Object);

        /// \brief Executes an entity editing command at the specified placement in the world.
        ///
        /// \param Command   The entity editing command to execute (e.g., add or remove).
        /// \param Placement The placement in the world where the entity command should be executed.
        /// \param Object    The unique identifier for the object to be added or removed.
        void ExecuteOnEntities(Command Command, Placement Placement, UInt32 Object);

        /// \brief Applies the specified command to a given area of tiles on a specific layer.
        ///
        /// \param Command The command to apply (e.g., add or remove).
        /// \param Area    The rectangular area of tiles to apply the command to, specified in tile coordinates.
        /// \param Layer   The layer of the tiles to apply the command to (e.g., base or detail).
        /// \param Handle  The unique identifier for the terrain type to apply when adding tiles.
        /// \param Span    The dimensions of the tile being applied.
        void ApplyTiles(Command Command, IntRect Area, Tile::Layer Layer, UInt16 Handle, IntVector2 Span);

        /// \brief Applies the specified operation to an area of tiles.
        ///
        /// \param Region    The region to apply the operation to.
        /// \param Operation The operation containing the details of the tile command to apply.
        void ApplyTiles(Ptr<Region> Region, ConstRef<OpTile> Operation);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Controller>  mController;
        Mode             mMode;
        Level            mLevel;
        Brush            mBrush;
        Sequence<OpTile> mOperations;
    };
}