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
#include "Tileon.Editor/Context.hpp"
#include "Tileon.Editor/Toolkit/Browser.hpp"
#include "Tileon.Editor/Toolkit/Previewer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Authors the slices an array texture holds, as a window of its own.
    ///
    /// \remark A slot is the identifier everything else stores, so slices are never renumbered: a slice that
    ///         is retired keeps both its place and its art, and only stops being offered.
    class TextureEditor final : public AssetEditor
    {
    public:

        /// \brief The extension the slice catalogue is written under, beside the array it describes.
        static constexpr Text kCatalogue = ".slices";

        /// \brief The sources a slice can be baked from, which is everything the texture baker imports.
        ///
        /// \remark A baked `.tex` is among them, so an array can be built out of one that already exists.
        static constexpr Text kSources   = ".png .jpg .jpeg .tga .bmp .hdr .tex";

    public:

        /// \brief Describes one slice of an array.
        struct Slot final
        {
            /// The art the slice is baked from, empty when it is carried over from the array as it stands.
            Str    Source;

            /// What the slice is called, which is all a palette has to tell one from another.
            Str    Name;

            /// The slice of the source the art is cut from, which is zero for anything but an array.
            UInt16 Slice   = 0;

            /// `true` when the slice is no longer offered, though it keeps both its place and its art.
            Bool   Retired = false;
        };

        /// \brief Reads the catalogue written beside an array, naming what each of its slices holds.
        ///
        /// \param Path The path on disk of the array, not of the catalogue.
        /// \return One entry per slice the catalogue describes, which may be fewer than the array holds.
        static Sequence<Slot> Read(Text Path);

    public:

        /// \brief Constructs the editor with the specified context.
        ///
        /// \param Context The context associated with this editor.
        explicit TextureEditor(Ref<Context> Context);

        /// \brief Opens an array with no slices yet, which the first one added gives an extent to.
        ///
        /// \param Path The path on disk the array will be written to.
        /// \param Key  The url the array is loaded under.
        /// \return `false` always, since nothing is written until the first slice gives it an extent.
        Bool Create(Text Path, AnyRef<Content::Uri> Key) override;

        /// \see AssetEditor::GetExtension()
        ZY_INLINE Text GetExtension() const override
        {
            return ".tex";
        }

        /// \brief Opens an array, reading the slices it already holds.
        ///
        /// \param Path The path on disk to read the array from.
        /// \param Key  The url the array is loaded under.
        void Open(Text Path, AnyRef<Content::Uri> Key) override;

        /// \brief Draws the editor, which does nothing while no array is open.
        ///
        /// \return `true` if the array was written this frame, `false` otherwise.
        Bool Draw() override;

    private:

        /// \brief Draws the table listing every slice the array holds.
        void DrawSlots();

        /// \brief Draws the pane previewing the slice in the selection.
        void DrawPreview();

        /// \brief Draws the prompt that stands between unbaked slices and closing the window.
        void DrawClosing();

        /// \brief Gets why the array cannot be baked as it stands.
        ///
        /// \return What is missing, or empty when nothing is.
        Text Reason() const;

        /// \brief Reads how many slices a baked array holds, and how big each of them is.
        ///
        /// \param Path   The path on disk of the array.
        /// \param Width  Receives the width of a slice, in texels.
        /// \param Height Receives the height of a slice, in texels.
        /// \param Slices Receives how many slices the array holds, which is never zero.
        /// \return `true` when the file is a baked texture and its header was read.
        static Bool Measure(Text Path, Ref<UInt16> Width, Ref<UInt16> Height, Ref<UInt16> Slices);

        /// \brief Appends one slot per slice the chosen source holds.
        ///
        /// \param Url The url the browser answered with, which names an asset rather than a file on disk.
        void Adopt(Text Url);

        /// \brief Gets the path on disk an asset is read from.
        ///
        /// \param Url The url the asset is loaded under.
        /// \return The path on disk.
        Str Locate(Text Url) const;

        /// \brief Reads the catalogue written beside the array, filling in what it does not describe.
        ///
        /// \param Layers The number of slices the array on disk was found to hold.
        void ReadCatalogue(UInt16 Layers);

        /// \brief Writes the catalogue beside the array.
        ///
        /// \return `true` if the catalogue was written, `false` otherwise.
        Bool WriteCatalogue() const;

        /// \brief Bakes the slices into the array and writes both it and its catalogue.
        ///
        /// \return `true` if the array was written, `false` otherwise.
        Bool Save();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Context>             mContext;
        Toolkit::Previewer       mPreview;
        Str                      mPath;
        Content::Uri             mKey;
        Sequence<Slot>           mSlots;
        Retainer<Graphic::Image> mImage;
        UInt16                   mExtentX;
        UInt16                   mExtentY;
        SInt32                   mSelection;
        Bool                     mOpen;
        Bool                     mDirty;
        Bool                     mClosing;
    };
}