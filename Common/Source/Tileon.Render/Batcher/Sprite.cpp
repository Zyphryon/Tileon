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

#include "Sprite.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Batcher
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Sprite::Sprite(ConstRetainer<Graphic::Service> Service, Ref<Render::Collector> Collector)
        : mService   { Service },
          mCollector { Collector },
          mPriority  { Render::Collector::Priority::Opaque }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Sprite::SetTechnique(ConstRetainer<Graphic::Technique> Technique,
        Graphic::Technique::Key Variant, Render::Collector::Priority Priority)
    {
        mTechnique = Technique;
        mVariant   = Variant;

        // One technique now serves both queues, so the open pass is the authority on which one this is; deriving
        // it from the base blend state would report the same queue for every variant.
        mPriority  = Priority;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Sprite::Draw(ConstRef<Appearance> Appearance, Vector2 Size, ConstRef<Matrix4x3> Transform, IntColor8 Tint)
    {
        ZY_ASSERT(mTechnique, "A technique must be set before recording a sprite");

        ConstRef<Graphic::Technique> Technique = (* mTechnique);

        Ref<Command> Entry = mCommands.Append();
        Entry.Layout.Transform = Matrix4x3::Pack(Transform);
        Entry.Layout.Frame     = Appearance.GetSource();
        Entry.Layout.Size      = Size;
        Entry.Layout.Color     = Tint;
        Entry.Layout.Facing    = Enum::Cast(Appearance.GetFacing());
        Entry.Material         = AddressOf(* Appearance.GetMaterial());
        Entry.Technique        = AddressOf(Technique);
        Entry.Variant          = mVariant;

        const Real32                    Order = Transform.GetColumn(2).GetW();
        const Render::Collector::Object Object(Enum::Cast(Batch::Sprite), mCommands.GetSize() - 1);
        mCollector.Push(Object, mPriority, Order,
            static_cast<UInt16>(mVariant), mTechnique->GetHandle(), Entry.Material->GetHandle());
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Sprite::Reset()
    {
        mCommands.Clear();
        mTechnique = nullptr;
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Sprite::Write(Ref<Render::Encoder> Encoder, ConstSpan<Render::Collector::Command> Commands)
    {
        // Every draw call in this batch shares the same technique and material.
        ConstRef<Command> First = mCommands[Commands.GetFront().Entry.Slot];

        const Graphic::Transient<Layout> Instances = Gather<Layout, Command>(* mService, mCommands, Commands);

        const Graphic::Invocation Invocation {
            .Count     = 4,
            .Instances = static_cast<UInt32>(Commands.GetSize())
        };
        Render::Encoder::Binder Binder = Encoder.Begin(* First.Technique);

        if (First.Material)
        {
            Binder.Apply(* First.Material);
        }
        Binder.SetVariant(First.Variant);
        Binder.Draw(Instances.GetStream(), Graphic::Stream(), Invocation);
    }
}