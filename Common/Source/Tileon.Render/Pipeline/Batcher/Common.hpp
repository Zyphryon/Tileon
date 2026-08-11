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
#include "Zyphryon.Math/Matrix4x3.hpp"
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

    /// \brief Defines a structure carrying a 4x3 transformation matrix in the layout the shaders consume.
    struct Matrix4x3Packed final
    {
        /// \brief The X row of the basis.
        Vector3 Basis0;

        /// \brief The translation along X.
        Real32  OffsetX;

        /// \brief The Y row of the basis.
        Vector3 Basis1;

        /// \brief The translation along Y.
        Real32  OffsetY;

        /// \brief The Z row of the basis.
        Vector3 Basis2;

        /// \brief The translation along Z.
        Real32  OffsetZ;

        /// \brief Sets the values of the matrix from a 4x3 transformation matrix.
        ///
        /// \param Matrix The 4x3 transformation matrix to convert into the packed format.
        /// \param Scale  The scale folded into the basis, leaving the translation where the transform put it.
        ZY_INLINE void SetData(ConstRef<Matrix4x3> Matrix, Real32 Scale = 1.0f)
        {
            const Vector4 Column0 = Matrix.GetColumn(0);
            const Vector4 Column1 = Matrix.GetColumn(1);
            const Vector4 Column2 = Matrix.GetColumn(2);

            Basis0  = Column0.GetXYZ() * Scale;
            OffsetX = Column0.GetW();
            Basis1  = Column1.GetXYZ() * Scale;
            OffsetY = Column1.GetW();
            Basis2  = Column2.GetXYZ() * Scale;
            OffsetZ = Column2.GetW();
        }
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
