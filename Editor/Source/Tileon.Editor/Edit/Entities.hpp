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
#include "Tileon.Editor/Panel/Viewport/Camera.hpp"
#include "Tileon.Render/Component.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Places, picks and moves the entities that sit on top of the ground.
    class Entities final
    {
    public:

        /// \brief How far above and below the ground a pick ray sweeps, in tiles.
        static constexpr Real32 kPickElevation  = Director::kElevation;

        /// \brief The share of its own reach a light is lifted by when it is placed without an elevation.
        static constexpr Real32 kLightElevation = 0.5f;

    public:

        /// \brief Constructs an entity tool editing the specified context's world.
        ///
        /// \param Context The reference to the context that the tool will interact with.
        Entities(Ref<Context> Context);

        /// \brief Executes an entity editing command at the specified placement in the world.
        ///
        /// \param Brush     The brush the command is issued with.
        /// \param Command   The entity editing command to execute (e.g., add or remove).
        /// \param Placement The placement in the world where the entity command should be executed.
        /// \param Object    The unique identifier for the object to be added or removed.
        void Execute(Brush Brush, Command Command, Placement Placement, UInt32 Object);


        /// \brief Shows where the specified archetype would land if it were placed at the given placement.
        ///
        /// \param Brush     The brush the preview is being dragged with.
        /// \param Placement The placement in the world the preview should follow.
        /// \param Object    The index of the archetype to preview.
        void UpdatePreview(Brush Brush, Placement Placement, UInt32 Object);

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
                Pose->SetScale(Vector3(Scale));
            }
        }

        /// \brief Rotates the pending placement preview about its origin.
        ///
        /// \param Delta The angle to add to the preview's current rotation.
        ZY_INLINE void AdjustPreviewRotation(Angle Delta)
        {
            if (const Ptr<Pose> Pose = mPreview.TryGet<Tileon::Pose>())
            {
                Pose->Rotate(Delta, Vector3::UnitY());
            }
        }

        /// \brief Gets the set of world entities currently selected, as instance-root ids.
        ///
        /// \return The current multi-selection.
        ZY_INLINE ConstRef<Bag<UInt64>> GetSelection() const
        {
            return mSelection;
        }

        /// \brief Mirrors external single-selection changes (Hierarchy, Inspector) into the multi-selection set.
        void ReconcileSelection();

        /// \brief Clears the entire selection.
        void ClearSelection();

        /// \brief Resolves the entity a click at a placement would select.
        ///
        /// \param Placement The placement in the world to pick from.
        /// \return The entity a click would take, or an invalid entity if there is none.
        Scene::Entity ResolveSelection(Placement Placement);

        /// \brief Replaces the selection with whatever sits under the placement, or clears it when empty.
        ///
        /// \param Placement The placement in the world to pick from.
        void SelectSingle(Placement Placement);

        /// \brief Adds or removes whatever sits under the placement from the selection.
        ///
        /// \param Placement The placement in the world to pick from.
        void SelectToggle(Placement Placement);

        /// \brief Selects every pickable entity whose centre falls inside the drawn screen-space box.
        ///
        /// \param Camera     The viewport projection used to place entities on screen.
        /// \param Minimum  The top-left corner of the box, in screen space.
        /// \param Maximum  The bottom-right corner of the box, in screen space.
        /// \param Additive `true` to add to the current selection, `false` to replace it.
        void SelectWithin(ConstRef<Camera> Camera, ImVec2 Minimum, ImVec2 Maximum, Bool Additive);

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

        /// \brief Checks whether an entity can be picked out of the buffer the view is resolving.
        ///
        /// \param Actor The entity to test.
        /// \return `true` when the entity shows in the current view, `false` otherwise.
        ZY_INLINE Bool IsPickable(Scene::Entity Actor)
        {
            const Bool IsLight = Actor.Has<Tileon::Glowlight>() || Actor.Has<Tileon::Spotlight>();

            switch (mContext.GetRenderer().GetOutput())
            {
            case Renderer::Target::Radiance:
                return IsLight;
            case Renderer::Target::Final:
                return true;
            default:
                return !IsLight;
            }
        }

        /// \brief Finds the frontmost entity that covers the specified placement.
        ///
        /// \param Placement The placement in the world to pick an entity from.
        /// \return The frontmost entity at the placement, or an invalid entity if none was found.
        ZY_INLINE Scene::Entity PickEntity(Placement Placement)
        {
            ConstRef<Director> Director = mContext.GetDirector();

            const Vector3 Ground = Vector3::FromXZ(
                Vector2(static_cast<Real32>(Placement.GetAbsoluteX()),
                        static_cast<Real32>(Placement.GetAbsoluteY())));
            const Vector3 Step  = Director.GetProjection().GetPickDirection();
            const Ray     Pick(Ground - Step * kPickElevation, Step);
            const Real32  Limit = Step.GetLength() * (2.0f * kPickElevation);

            Scene::Entity Result;
            Real32        Nearest = 0.0f;

            mContext.GetSupervisor().QueryRay(Pick, Limit, [&](Scene::Entity Actor, Real32 Distance)
            {
                const ConstPtr<Transform> Transform = Actor.TryGet<const Tileon::Transform>();

                if (!Transform || !IsPickable(Actor))
                {
                    return;
                }

                const Vector3 World = Vector3(Transform->GetOrigin()) + Transform->GetWorldspace().GetTranslation();
                const Real32  Sort  = Director.GetProjection().GetDepth(World);

                if (!Result.IsValid() || Sort < Nearest)
                {
                    Result  = Actor;
                    Nearest = Sort;
                }
            });
            return Result;
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

        /// \brief Gets the absolute placement of an entity's origin, from its pose and owning region.
    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Context>  mContext;
        Scene::Entity mPreview;
        Bag<UInt64>   mSelection;
        UInt64        mSelectionPrimary;
        UInt32        mRevision;
        Blob          mClipboard;
        UInt32        mClipboardCount;
    };
}