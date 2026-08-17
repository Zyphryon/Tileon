// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Context.hpp"
#include <Zyphryon.Scene/Codec.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    History::History(Ref<Context> Context)
        : mContext  { Context },
          mCursor   { 0 },
          mDepth    { 0 },
          mSequence { 0 },
          mRevision { 0 },
          mStale    { false }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void History::Open(Text Label)
    {
        if (mDepth++ == 0)
        {
            mPending.Label = Label;
            mPending.Changes.Clear();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void History::Close()
    {
        if (mDepth == 0 || --mDepth > 0)
        {
            return;
        }

        // Read the state each subject was left in, which is the state redoing the step restores.
        for (Ref<Change> Entry : mPending.Changes)
        {
            if (Entry.Resolved)
            {
                continue;
            }

            switch (Entry.Target)
            {
            case Target::Entity:
                Entry.After = SaveEntity(Resolve(Entry.Key));
                break;
            case Target::Singleton:
                Entry.After = SaveSingleton(mContext.GetScene().GetEntity(Entry.Key));
                break;
            }
            Entry.Resolved = true;
        }

        // An edit that left everything exactly as it found it does not deserve an entry in the stack.
        mPending.Changes.RemoveIf([](ConstRef<Change> Entry)
        {
            return IsSame(Entry.Before, Entry.After);
        });

        if (mPending.Changes.IsEmpty())
        {
            mPending.Changes.Clear();
            return;
        }

        // A fresh edit invalidates everything that was taken back, since the branch it belonged to is now gone.
        if (mCursor < mSteps.GetSize())
        {
            mSteps.Remove(mCursor, mSteps.GetSize() - mCursor);
        }

        mSteps.Append(Move(mPending));

        if (mSteps.GetSize() > kMaxSteps)
        {
            mSteps.Remove(0);
        }

        mCursor  = static_cast<UInt32>(mSteps.GetSize());
        mPending = Step();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void History::CaptureEntity(Scene::Entity Actor)
    {
        if (!Actor.IsValid() || !Actor.IsAlive())
        {
            return;
        }

        if (const Ptr<Change> Entry = Reach(Target::Entity, Track(Actor)); Entry && !Entry->Captured)
        {
            Entry->Before   = SaveEntity(Actor);
            Entry->Captured = true;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void History::RecordEntity(Scene::Entity Actor)
    {
        if (Actor.IsValid() && Actor.IsAlive())
        {
            Reach(Target::Entity, Track(Actor));
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void History::DiscardEntity(Scene::Entity Actor)
    {
        if (!Actor.IsValid() || !Actor.IsAlive())
        {
            return;
        }

        if (const Ptr<Change> Entry = Reach(Target::Entity, Track(Actor)))
        {
            if (!Entry->Captured)
            {
                Entry->Before   = SaveEntity(Actor);
                Entry->Captured = true;
            }

            // Disposal is deferred, so the entity is still alive when the step closes; say now that it is going.
            Entry->After    = Blob();
            Entry->Resolved = true;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void History::CaptureSingleton(Scene::Entity Component)
    {
        if (!Component.IsValid())
        {
            return;
        }

        if (const Ptr<Change> Entry = Reach(Target::Singleton, Component.GetID()); Entry && !Entry->Captured)
        {
            Entry->Before   = SaveSingleton(Component);
            Entry->Captured = true;
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void History::Undo()
    {
        if (CanUndo())
        {
            Apply(mSteps[--mCursor], true);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void History::Redo()
    {
        if (CanRedo())
        {
            Apply(mSteps[mCursor++], false);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void History::Clear()
    {
        Forget();

        mPending.Changes.Clear();
        mDepth = 0;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void History::Forget()
    {
        mSteps.Clear();
        mTokens.Clear();
        mEntities.Clear();
        mRegions.Clear();
        mRemap.Clear();

        mCursor = 0;
        mStale  = false;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool History::Bind(UInt64 Key, Scene::Entity Actor)
    {
        const UInt64 ID = Actor.GetID();

        if (const ConstPtr<UInt64> Known = mRegions.Find(Key); Known && (* Known) != ID)
        {
            return false;
        }
        mRegions.Assign(Key, ID);
        return true;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt64 History::Remap(UInt64 Entity) const
    {
        const ConstPtr<UInt64> Match = mRemap.Find(Entity);
        return Match ? (* Match) : Entity;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Ptr<History::Change> History::Reach(Target Target, UInt64 Key)
    {
        if (mDepth == 0)
        {
            return nullptr;
        }

        for (Ref<Change> Entry : mPending.Changes)
        {
            if (Entry.Target == Target && Entry.Key == Key)
            {
                return & Entry;
            }
        }
        return & mPending.Changes.Append(Change(Target, Key, false, false, Blob(), Blob()));
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    UInt64 History::Track(Scene::Entity Actor)
    {
        const UInt64 ID = Actor.GetID();

        if (const ConstPtr<UInt64> Existing = mTokens.Find(ID))
        {
            return * Existing;
        }

        const UInt64 Token = ++mSequence;
        mTokens.Assign(ID, Token);
        mEntities.Assign(Token, ID);
        return Token;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Entity History::Resolve(UInt64 Token) const
    {
        const ConstPtr<UInt64> Entity = mEntities.Find(Token);
        return (Entity && * Entity) ? mContext.GetScene().GetEntity(* Entity) : Scene::Entity();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void History::Rebind(UInt64 Token, UInt64 Entity)
    {
        if (const ConstPtr<UInt64> Previous = mEntities.Find(Token); Previous && * Previous)
        {
            mRemap.Assign(* Previous, Entity);
            mTokens.Erase(* Previous);
        }

        mEntities.Assign(Token, Entity);

        if (Entity)
        {
            mTokens.Assign(Entity, Token);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob History::SaveEntity(Scene::Entity Actor)
    {
        if (!Actor.IsValid() || !Actor.IsAlive())
        {
            return Blob();
        }

        // Only the region the entity was placed in gives its pose a place in the world, so it is stored alongside.
        const Scene::Entity            Region    = FindRegion(Actor);
        const ConstPtr<Tileon::Region> Component = Region.IsValid() ? Region.TryGet<const Tileon::Region>() : nullptr;

        if (!Component)
        {
            return Blob();
        }

        // The region came back from disk since the stack was built, so what it holds is no longer a step behind.
        if (!Bind(GetKey(Component->GetX(), Component->GetY()), Region))
        {
            Forget();
        }

        Writer Output;
        Output.Write<SInt16>(Component->GetX());
        Output.Write<SInt16>(Component->GetY());
        mContext.GetScene().SaveHierarchy(Output, Actor);
        return Output.Detach();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void History::ApplyEntity(UInt64 Token, ConstRef<Blob> State)
    {
        // Whatever stands in the entity's place goes, since a restore rebuilds it from the state it was saved in.
        if (const Scene::Entity Current = Resolve(Token); Current.IsValid() && Current.IsAlive())
        {
            if (const Scene::Entity Region = FindRegion(Current); Region.IsValid())
            {
                Region.Add<Persist>();
            }
            Current.Add<Dispose>();
        }

        Scene::Entity Restored;

        if (State != nullptr)
        {
            Reader       Input(State.GetData(), State.GetSize());
            const SInt16 RegionX = Input.Read<SInt16>();
            const SInt16 RegionY = Input.Read<SInt16>();

            const Scene::Entity Region = mContext.GetSupervisor().GetOrLoadRegion(RegionX, RegionY, true);

            // A region back from disk already carries its own copy of the entity, so restoring would double it.
            if (Region.IsValid() && !Bind(GetKey(RegionX, RegionY), Region))
            {
                mStale = true;
                return;
            }

            if (Region.IsValid())
            {
                Restored = mContext.GetScene().LoadHierarchy(Input);
                Restored.Attach(Region, Scene::Hierarchy::Open);
                Restored.Add<Stale>();
                Restored.Add<Persist>();
                Region.Add<Persist>();
            }
        }

        // One rebind for the whole swap, so the id the entity left behind maps straight onto the one it took.
        Rebind(Token, Restored.IsValid() ? Restored.GetID() : 0);
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Blob History::SaveSingleton(Scene::Entity Component)
    {
        // A singleton is its own owner: the world holds it when the component entity carries its own component.
        const Bool Present = Component.Has(Component);

        Writer Output;
        Output.Write<UInt8>(Present ? 1 : 0);

        if (Present)
        {
            Scene::Codec::WriteComponent<Scene::Entity>(Output, Component, Component);
        }
        return Output.Detach();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void History::ApplySingleton(UInt64 Key, ConstRef<Blob> State)
    {
        const Scene::Entity Component = mContext.GetScene().GetEntity(Key);

        if (State == nullptr || !Component.IsValid())
        {
            return;
        }

        Reader Input(State.GetData(), State.GetSize());

        if (Input.Read<UInt8>())
        {
            // A singleton that was never declared serializable writes no payload, so only its presence comes back.
            if (Input.GetAvailable() > 0)
            {
                Scene::Codec::ReadComponent<Scene::Entity>(Component.GetWorld(), Input, Component);
            }
            else
            {
                Component.Add(Component);
            }
        }
        else
        {
            Component.Remove(Component);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void History::Apply(Ref<Step> Step, Bool Backward)
    {
        mRemap.Clear();

        const UInt Count = Step.Changes.GetSize();

        for (UInt Index = 0; Index < Count && !mStale; ++Index)
        {
            // Taking a step back walks its changes back to front, so it unwinds in the order it was made.
            Ref<Change>    Entry = Step.Changes[Backward ? Count - 1 - Index : Index];
            ConstRef<Blob> State = Backward ? Entry.Before : Entry.After;

            switch (Entry.Target)
            {
            case Target::Entity:
                ApplyEntity(Entry.Key, State);
                break;
            case Target::Singleton:
                ApplySingleton(Entry.Key, State);
                break;
            }
        }

        if (!mRemap.IsEmpty())
        {
            ++mRevision;

            // The selection is held by id, so anything it points at that was rebuilt has to follow it.
            if (const UInt64 Selection = static_cast<UInt64>(mContext.GetInteger("Selection.Entity", 0)))
            {
                mContext.SetInteger("Selection.Entity", static_cast<SInt64>(Remap(Selection)));
            }
        }

        if (mStale)
        {
            LOG_W("History: a region was reloaded from disk, so the recorded steps no longer apply");

            Forget();
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Scene::Entity History::FindRegion(Scene::Entity Actor)
    {
        for (Scene::Entity Cursor = Actor; Cursor.IsValid(); Cursor = Cursor.GetParent())
        {
            if (Cursor.Has<Region>())
            {
                return Cursor;
            }
        }
        return Scene::Entity();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Bool History::IsSame(ConstRef<Blob> Left, ConstRef<Blob> Right)
    {
        if (Left.GetSize() != Right.GetSize())
        {
            return false;
        }
        return Left.GetSize() == 0 || Compare(Left.GetData(), Right.GetData(), Left.GetSize());
    }
}