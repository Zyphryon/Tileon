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

#include "Tileon.Editor/Asset/AssetEditor.hpp"
#include <Tileon.Render/Types.hpp>
#include "Tileon.Editor/Context.hpp"
#include <Zyphryon.Graphic/Types.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Authors the textures and values a material hands its technique, as a window of its own.
    class MaterialEditor final : public AssetEditor
    {
    public:

        /// \brief Constructs the editor with the specified context.
        ///
        /// \param Context The context associated with this editor.
        MaterialEditor(Ref<Context> Context);

        /// \brief Writes an empty material and opens it for editing.
        ///
        /// \param Path The path on disk to write the material to.
        /// \param Key  The url the material is loaded under.
        /// \return `true` if the material was written, `false` otherwise.
        Bool Create(Text Path, AnyRef<Content::Uri> Key) override;

        /// \see AssetEditor::GetExtension()
        ZY_INLINE Text GetExtension() const override
        {
            return ".mtl";
        }

        /// \brief Opens a material, reading what it already binds.
        ///
        /// \param Path The path on disk to read the material from.
        /// \param Key  The url the material is loaded under.
        void Open(Text Path, AnyRef<Content::Uri> Key) override;

        /// \brief Draws the editor, which does nothing while no material is open.
        ///
        /// \return `true` if the material was written this frame, `false` otherwise.
        Bool Draw() override;

        /// \brief Describes one texture a material binds, together with the sampler that reads it.
        struct Binding final
        {
            /// The name the technique declares the texture under.
            Str                     Name;

            /// The texture the material binds, relative to the mount.
            Str                     Path;

            /// Whether the technique's own sampler stands, which leaves everything below unwritten.
            Bool                    Inherit  = false;

            /// How the sampler reads between texels.
            Graphic::TextureFilter  Filter   = Graphic::TextureFilter::Point;

            /// How the sampler reads past the horizontal edges.
            Graphic::TextureAddress AddressU = Graphic::TextureAddress::Clamp;

            /// How the sampler reads past the vertical edges.
            Graphic::TextureAddress AddressV = Graphic::TextureAddress::Clamp;

            /// How the sampler reads past the depth edges, which only a volume ever has.
            Graphic::TextureAddress AddressW = Graphic::TextureAddress::Clamp;
        };

        /// \brief Describes one value a material hands its technique.
        struct Constant final
        {
            /// The name the technique declares the value under.
            Str              Name;

            /// What the value is, which decides which of the stores below carries it.
            Graphic::Uniform Type    = Graphic::Uniform::Float;

            /// The value of a boolean.
            Bool             Boolean = false;

            /// The components of a real value, of which the type decides how many are read.
            Array<Real32, 4> Decimal;

            /// The components of an integer value, of which the type decides how many are read.
            Array<SInt32, 4> Integer;

            /// \brief Constructs a constant of a single float, left at zero.
            ZY_INLINE Constant()
            {
                Decimal.Fill(0.0f);
                Integer.Fill(0);
            }
        };

        /// \brief Writes a material binding the given textures and handing over the given values.
        ///
        /// \param Path      The path on disk to write the material to.
        /// \param Bindings  The textures the material binds.
        /// \param Constants The values the material hands its technique.
        /// \return `true` if the material was written, `false` otherwise.
        static Bool Write(Text Path, ConstSpan<Binding> Bindings, ConstSpan<Constant> Constants = { });

    private:

        /// \brief Draws the prompt that stands between unsaved edits and closing the window.
        void DrawClosing();

        /// \brief Gets why the material cannot be written as it stands.
        ///
        /// \return What is wrong with it, or empty when nothing is.
        Text Reason() const;

        /// \brief Writes the material being edited back over the file it was read from, then reloads it.
        ///
        /// \return `true` if the material was written, `false` otherwise.
        Bool Save();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Context>       mContext;
        Str                mPath;
        Content::Uri       mKey;
        Sequence<Binding>  mBindings;
        Sequence<Constant> mConstants;
        Bool               mDirty;
        Bool               mClosing;
    };
}