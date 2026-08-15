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

#include <Tileon.World/Region.hpp>
#include <Zyphryon.Scene/Entity.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    class Context;

    /// \brief Records what an edit did to the world, so it can be taken back and put back.
    ///
    /// \note Restoring an entity destroys whatever stands in its place and deserializes it anew,
    /// which gives it an id it did not have before.
    class History final
    {
    public:

        /// \brief The number of steps kept before the oldest one is forgotten.
        static constexpr UInt32 kMaxSteps = 64;

    public:

        /// \brief Constructs a history bound to the specified context.
        ///
        /// \param Context The context whose world the recorded steps are applied to.
        explicit History(Ref<Context> Context);

        /// \brief Retries the restores that were waiting on a region to finish streaming in.
        void Tick();

        /// \brief Opens a step, so everything recorded until it closes is undone and redone as one.
        ///
        /// \param Label The name the step is offered under, such as `Paint` or `Move`.
        void Open(Text Label);

        /// \brief Closes the step opened by \ref Open, discarding it when the edit changed nothing.
        void Close();

        /// \brief Records an entity's state as it is now, which is the state undoing the step restores.
        ///
        /// \param Actor The entity that is about to be changed.
        void CaptureEntity(Scene::Entity Actor);

        /// \brief Records an entity as part of the step, whose state when the step closes is what redoing restores.
        ///
        /// \param Actor The entity that was created or changed.
        void RecordEntity(Scene::Entity Actor);

        /// \brief Records an entity that is going away, so undoing the step brings it back.
        ///
        /// Disposal is deferred, so this must be called while the entity still holds the state to remember.
        ///
        /// \param Actor The entity that is about to be destroyed.
        void DiscardEntity(Scene::Entity Actor);

        /// \brief Records a singleton as it stands now, which is what undoing the step restores.
        ///
        /// \param Component The component entity of the singleton that is about to be changed.
        void CaptureSingleton(Scene::Entity Component);

        /// \brief Records a region's tiles as they are now, which is what undoing the step restores.
        ///
        /// The whole region is taken, so a stroke that paints it over and over only ever costs one pair of states.
        ///
        /// \param Actor The region entity that is about to be painted.
        void CaptureRegion(Scene::Entity Actor);

        /// \brief Takes back the most recent step.
        void Undo();

        /// \brief Puts back the most recently taken back step.
        void Redo();

        /// \brief Forgets every recorded step.
        void Clear();

        /// \brief Returns whether there is a step to take back.
        ///
        /// \return `true` if a step can be undone, `false` otherwise.
        ZY_INLINE Bool CanUndo() const
        {
            return mCursor > 0;
        }

        /// \brief Returns whether there is a step to put back.
        ///
        /// \return `true` if a step can be redone, `false` otherwise.
        ZY_INLINE Bool CanRedo() const
        {
            return mCursor < mSteps.GetSize();
        }

        /// \brief Gets the name of the step \ref Undo would take back.
        ///
        /// \return The step's label, or an empty text when there is nothing to undo.
        ZY_INLINE Text GetUndoLabel() const
        {
            return CanUndo() ? Text(mSteps[mCursor - 1].Label) : Text();
        }

        /// \brief Gets the name of the step \ref Redo would put back.
        ///
        /// \return The step's label, or an empty text when there is nothing to redo.
        ZY_INLINE Text GetRedoLabel() const
        {
            return CanRedo() ? Text(mSteps[mCursor].Label) : Text();
        }

        /// \brief Translates an entity id that an undone or redone step has since replaced.
        ///
        /// \param Entity The id to translate.
        /// \return The id the entity now carries, or `0` when it no longer exists.
        UInt64 Remap(UInt64 Entity) const;

        /// \brief Gets a counter bumped every time a step replaced live entity ids.
        ///
        /// \return The current revision, which a view compares against its own to know when to remap.
        ZY_INLINE UInt32 GetRevision() const
        {
            return mRevision;
        }

    private:

        /// \brief Identifies what a change addresses.
        enum class Target : UInt8
        {
            Entity,    ///< An entity, addressed by the token bound to it.
            Region,    ///< A region's tiles, addressed by its packed coordinates.
            Singleton, ///< A singleton the world holds, addressed by its component entity.
        };

        /// \brief One thing a step changed, held as the state on either side of the edit.
        struct Change final
        {
            /// What the change addresses.
            Target Target;

            /// The token of the entity, the packed coordinates of the region, or the component entity id.
            UInt64 Key;

            /// Whether the state before the edit has been taken, which only the first capture of a step does.
            Bool   Captured;

            /// Whether the state after the edit is already final, or is still to be read when the step closes.
            Bool   Resolved;

            /// The state before the edit, which is empty when the subject did not exist yet.
            Blob   Before;

            /// The state after the edit, which is empty when the subject no longer exists.
            Blob   After;
        };

        /// \brief One entry in the undo stack, holding every change an edit made.
        struct Step final
        {
            /// The name the step is offered under.
            String<32>       Label;

            /// The changes the edit made, in the order they were recorded.
            Sequence<Change> Changes;
        };

        /// \brief A restore that is waiting on its region to finish streaming in.
        struct Deferred final
        {
            /// The packed coordinates of the region to restore.
            UInt64 Key;

            /// The tiles to write once the region is resident.
            Blob   State;
        };

        /// \brief Packs a region's coordinates into the key a change addresses it by.
        ///
        /// \param X The x-coordinate of the region.
        /// \param Y The y-coordinate of the region.
        /// \return The packed key.
        ZY_INLINE static constexpr UInt64 GetKey(SInt16 X, SInt16 Y)
        {
            return (static_cast<UInt64>(static_cast<UInt16>(X)) << 16) | static_cast<UInt16>(Y);
        }

        /// \brief Finds the change a step holds for a subject, appending a fresh one when it holds none yet.
        ///
        /// \param Target The kind of subject the change addresses.
        /// \param Key    The token or packed coordinates that identify the subject.
        /// \return The change, or `nullptr` when no step is open.
        Ptr<Change> Reach(Target Target, UInt64 Key);

        /// \brief Checks a region against the one the recorded steps were made against, and adopts it when new.
        ///
        /// A region that streamed out and back in is a different entity holding a copy read from disk, which the
        /// recorded steps know nothing about; restoring into it would duplicate what it already carries.
        ///
        /// \param Key   The packed coordinates of the region.
        /// \param Actor The region entity as it stands now.
        /// \return `true` when the region is the one the steps were made against, `false` when it was replaced.
        Bool Bind(UInt64 Key, Scene::Entity Actor);

        /// \brief Forgets every recorded step without disturbing the step that is currently open.
        void Forget();

        /// \brief Gets the token bound to an entity, minting one when it has none yet.
        ///
        /// \param Actor The entity to address.
        /// \return The token that now stands for the entity.
        UInt64 Track(Scene::Entity Actor);

        /// \brief Gets the entity a token currently stands for.
        ///
        /// \param Token The token to resolve.
        /// \return The entity, or an invalid entity when the token stands for nothing.
        Scene::Entity Resolve(UInt64 Token) const;

        /// \brief Binds a token to the entity that now carries it, releasing whatever it was bound to.
        ///
        /// \param Token  The token to rebind.
        /// \param Entity The id the token now stands for, or `0` when the entity is gone.
        void Rebind(UInt64 Token, UInt64 Entity);

        /// \brief Serializes an entity and the region it was placed in.
        ///
        /// \param Actor The entity to serialize.
        /// \return The entity's state, which is empty when it is not a placed entity.
        Blob SaveEntity(Scene::Entity Actor);

        /// \brief Restores an entity to a serialized state, replacing whatever the token stands for.
        ///
        /// \param Token The token that identifies the entity.
        /// \param State The state to restore, which is empty to leave the entity destroyed.
        void ApplyEntity(UInt64 Token, ConstRef<Blob> State);

        /// \brief Serializes whether the world holds a singleton, and the value it holds.
        ///
        /// \param Component The component entity of the singleton to serialize.
        /// \return The singleton's state.
        static Blob SaveSingleton(Scene::Entity Component);

        /// \brief Restores a singleton to a serialized state, attaching or detaching it as that state says.
        ///
        /// \param Key   The component entity id of the singleton.
        /// \param State The state to restore.
        void ApplySingleton(UInt64 Key, ConstRef<Blob> State);

        /// \brief Serializes every tile of a region.
        ///
        /// \param Component The region to serialize.
        /// \return The region's tiles.
        Blob SaveRegion(ConstRef<Region> Component) const;

        /// \brief Restores every tile of a region, queueing the write when the region is not resident yet.
        ///
        /// \param Key   The packed coordinates of the region.
        /// \param State The tiles to write.
        void ApplyRegion(UInt64 Key, ConstRef<Blob> State);

        /// \brief Writes a serialized set of tiles into a resident region.
        ///
        /// \param Actor The region entity that owns the tiles.
        /// \param State The tiles to write.
        static void WriteRegion(Scene::Entity Actor, ConstRef<Blob> State);

        /// \brief Applies one side of every change a step holds.
        ///
        /// \param Step     The step to apply.
        /// \param Backward `true` to restore the state from before the edit, `false` for the state after it.
        void Apply(Ref<Step> Step, Bool Backward);

        /// \brief Gets the entity of the region an entity was placed in.
        ///
        /// \param Actor The entity to locate.
        /// \return The region that holds the entity, or an invalid entity when it sits outside one.
        static Scene::Entity FindRegion(Scene::Entity Actor);

        /// \brief Compares two serialized states byte for byte.
        ///
        /// \param Left  The first state.
        /// \param Right The second state.
        /// \return `true` when both states are identical, `false` otherwise.
        static Bool IsSame(ConstRef<Blob> Left, ConstRef<Blob> Right);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Context>          mContext;
        Sequence<Step>        mSteps;
        UInt32                mCursor;
        UInt32                mDepth;
        Step                  mPending;
        Sequence<Deferred>    mDeferred;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Table<UInt64, UInt64> mTokens;
        Table<UInt64, UInt64> mEntities;
        Table<UInt64, UInt64> mRegions;
        Table<UInt64, UInt64> mRemap;
        UInt64                mSequence;
        UInt32                mRevision;
        Bool                  mStale;
    };
}