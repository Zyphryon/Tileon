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

#include "Zyphryon.Graphic/Service.hpp"
#include "Zyphryon.Render/Collector.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Enumerates the kinds of draw the collector dispatches back when drained.
    enum class Batch : UInt8
    {
        Sprite,         ///< A sprite for drawing textured quads.
        Glyph,          ///< A text glyph for drawing characters from a font atlas.
    };

    /// \brief Gathers the per-instance input of a batch's draw calls into a transient instance stream.
    ///
    /// \param Service  The service the transient stream is allocated from.
    /// \param Source   The commands the batch indexes into.
    /// \param Commands The span of draw calls forming the batch.
    /// \return The stream holding the batch's per-instance input, in the batch's own order.
    template<typename Layout, typename Command>
    ZY_INLINE Graphic::Transient<Layout> Gather(
        Ref<Graphic::Service> Service, ConstSpan<Command> Source, ConstSpan<Render::Collector::Command> Commands)
    {
        Graphic::Transient<Layout> Instances = Service.AllocateInFlightVertices<Layout>(Commands.GetSize());

        for (UInt32 Element = 0; Element < Commands.GetSize(); ++Element)
        {
            Instances[Element] = Source[Commands[Element].Entry.Slot].Layout;
        }
        return Instances;
    }
}