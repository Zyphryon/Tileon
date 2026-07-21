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

#include "Tileon.Editor/Context.hpp"
#include <Zyphryon.Base/Container/Bag.hpp>
#include <Zyphryon.Base/Memory/Blob.hpp>

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

        /// \brief Constructs a workshop with the specified context reference.
        ///
        /// \param Context The reference to the context that the workshop will interact with.
        Workshop(Ref<Context> Context);

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

        /// \brief Shows where the specified archetype would land if it were placed at the given placement.
        ///
        /// \param Placement The placement in the world the preview should follow.
        /// \param Object    The index of the archetype to preview.
        void UpdatePreview(Placement Placement, UInt32 Object);

        /// \brief Discards the preview, if one is currently being shown.
        void ClearPreview();

        /// \brief Returns whether a placement preview is currently active.
        ///
        /// \return `true` if a preview entity exists, `false` otherwise.
        ZY_INLINE Bool HasPreview() const
        {
            return mPreview.IsAlive();
        }

        /// \brief Grows or shrinks the pending placement preview about its origin.
        ///
        /// \param Steps The signed number of wheel steps; positive enlarges, negative shrinks.
        ZY_INLINE void AdjustPreviewScale(Real32 Steps)
        {
            if (const Ptr<Pose> Pose = mPreview.TryGet<Tileon::Pose>())
            {
                const Real32 Scale = Max(Pose->GetScale().GetX() * Pow(1.1f, Steps), 0.1f);
                Pose->SetScale(Vector2(Scale));
            }
        }

        /// \brief Rotates the pending placement preview about its origin.
        ///
        /// \param Delta The angle to add to the preview's current rotation.
        ZY_INLINE void AdjustPreviewRotation(Angle Delta)
        {
            if (const Ptr<Pose> Pose = mPreview.TryGet<Tileon::Pose>())
            {
                Pose->SetRotation(Pose->GetRotation() + Delta);
            }
        }

        /// \brief Gets the set of world entities currently selected, as instance-root ids.
        ///
        /// \return The current multi-selection.
        ZY_INLINE ConstRef<Bag<UInt64>> GetSelection() const
        {
            return mSelection;
        }

        /// \brief Returns whether the clipboard holds a group that can be pasted.
        ///
        /// \return `true` if a copy or cut has populated the clipboard, `false` otherwise.
        ZY_INLINE Bool HasClipboard() const
        {
            return mClipboardCount > 0;
        }

        /// \brief Mirrors external single-selection changes (Hierarchy, Inspector) into the multi-selection set.
        void ReconcileSelection();

        /// \brief Clears the entire selection.
        void ClearSelection();

        /// \brief Replaces the selection with whatever sits under the placement, or clears it when empty.
        ///
        /// \param Placement The placement in the world to pick from.
        void SelectSingle(Placement Placement);

        /// \brief Adds or removes whatever sits under the placement from the selection.
        ///
        /// \param Placement The placement in the world to pick from.
        void SelectToggle(Placement Placement);

        /// \brief Selects every pickable entity whose bound overlaps the given area.
        ///
        /// \param Area     The world-tile rectangle to select within.
        /// \param Additive `true` to add to the current selection, `false` to replace it.
        void SelectWithin(IntRect Area, Bool Additive);

        /// \brief Destroys every selected entity and clears the selection.
        void DeleteSelection();

        /// \brief Serializes the selection into the clipboard, preserving intra-group offsets.
        void CopySelection();

        /// \brief Copies the selection into the clipboard, then deletes it.
        void CutSelection();

        /// \brief Instantiates the clipboard's group at the given placement and selects the result.
        ///
        /// \param Placement The placement the group's anchor should land on.
        void Paste(Placement Placement);

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

        /// \brief Finds the frontmost entity that covers the specified placement.
        ///
        /// \param Placement The placement in the world to pick an entity from.
        /// \return The frontmost entity at the placement, or an invalid entity if none was found.
        ZY_INLINE Scene::Entity PickEntity(Placement Placement)
        {
            const Real32 AbsX  = static_cast<Real32>(Placement.GetAbsoluteX());
            const Real32 AbsY  = static_cast<Real32>(Placement.GetAbsoluteY());
            const SInt32 TileX = Floor(AbsX);
            const SInt32 TileY = Floor(AbsY);

            const IntRect Area(TileX, TileY, TileX + 1, TileY + 1);
            return mContext.GetSupervisor().QueryFrontmost(Area, Vector2(AbsX, AbsY));
        }

        /// \brief Instantiates an archetype into the region that owns the specified placement.
        ///
        /// \param Placement The placement in the world where the entity should be created.
        /// \param Object    The index of the archetype to instantiate.
        void AddEntity(Placement Placement, UInt32 Object);

        /// \brief Creates the preview instance if needed, then poses it at the specified placement.
        ///
        /// The instance is a fully-fledged archetype instance minus the traits that would make the rest of the
        /// world treat it as placed, which is what \ref AddEntity grants back when it promotes the preview.
        ///
        /// \param Actor     The region that owns the placement, which the preview is parented into.
        /// \param Archetype The archetype the preview mirrors.
        /// \param Placement The placement in the world the preview should sit at.
        /// \return The preview instance.
        Scene::Entity EnsurePreview(Scene::Entity Actor, Scene::Entity Archetype, Placement Placement);

        /// \brief Removes the frontmost entity found at the specified placement.
        ///
        /// \param Placement The placement in the world to remove an entity from.
        void RemoveEntity(Placement Placement);

        /// \brief Selects the frontmost entity found at the specified placement, clearing the selection if empty.
        ///
        /// \param Placement The placement in the world to select an entity from.
        void SelectEntity(Placement Placement);

        /// \brief Records the selection primary in the shared context key, keeping single-selection views in sync.
        ///
        /// \param Entity The id to publish as the primary selected entity, or `0` for none.
        void SetPrimary(UInt64 Entity);

        /// \brief Gets the absolute placement of an entity's origin, from its pose and owning region.
        ///
        /// \param Actor The entity to locate.
        /// \return The entity's absolute placement.
        Placement OriginOf(Scene::Entity Actor) const;

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

        Ref<Context>     mContext;
        Mode             mMode;
        Level            mLevel;
        Brush            mBrush;
        Sequence<OpTile> mOperations;
        Scene::Entity    mPreview;
        Bag<UInt64>      mSelection;
        UInt64           mSelectionPrimary;
        Blob             mClipboard;
        UInt32           mClipboardCount;
    };
}