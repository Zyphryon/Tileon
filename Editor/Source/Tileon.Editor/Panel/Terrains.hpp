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

#include "Tileon.Editor/Panel.hpp"
#include "Tileon.Editor/Widget/Browser.hpp"
#include "Tileon.Editor/Widget/Previewer.hpp"
#include <Tileon.Render/Tileset.hpp>
#include <Baker.Texture/Baker.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Provides tools and functionality for managing and editing the tileset in the editor.
    ///
    /// \note Every change rebuilds a sheet and renumbers every run in the same breath, so a placement can
    ///       never name a slice that has moved out from under it.
    class Terrains final : public Panel
    {
    public:

        /// \brief The sheet a motif is fired into when the project has not named another.
        static constexpr Symbol kSheet = "Resources://Tileset/Tileset{0}.{1}.tex";

    public:

        /// \brief Constructs the activity with the specified context.
        ///
        /// \param Context The context associated with this activity.
        Terrains(Ref<Context> Context);

        /// \see Panel::OnDraw()
        void OnDraw() override;

    private:

        /// Shorthand for the manifest entry naming one frame, and the slice it is cut from.
        using Entry = ::Pipeline::Baker::Texture::Manifest::Entry;

        /// \brief Names the art one motif takes on at the next firing.
        struct Change final
        {
            /// \brief Constructs a change that takes a motif's art away.
            ZY_INLINE Change()
                : Motif  { 0 },
                  Sheet  { 0 },
                  Fired  { false },
                  Frames { 0 },
                  Width  { 0 },
                  Height { 0 }
            {
            }

            /// The motif the change applies to.
            UInt16     Motif;

            /// The sheet its frames are fired into.
            UInt16     Sheet;

            /// Whether the change names a sheet at all, which a discard does not.
            Bool       Fired;

            /// The image its frames are cut from, which is empty when the art is taken away.
            Str        Source;

            /// The number of frames it takes.
            UInt16     Frames;

            /// The corner of the first frame on the source, in texels.
            IntVector2 Origin;

            /// The width of a frame, in texels.
            UInt16     Width;

            /// The height of a frame, in texels.
            UInt16     Height;
        };

        /// \brief One sheet's worth of frames, gathered in the order the motifs hold them.
        struct Batch final
        {
            /// The frames the sheet holds, one entry per slice.
            Sequence<Entry> Entries;
        };

    private:

        /// \brief Draws the list panel of the panel.
        void DrawListPanel();

        /// \brief Draws the left panel of the panel.
        ///
        /// \param Terrain The currently selected terrain.
        /// \param Motif   The motif data associated with the selected terrain.
        void DrawLeftPanel(Ref<Terrain> Terrain, Ref<Motif> Motif);

        /// \brief Draws the art panel, which fires a motif's frames into a sheet or takes them away.
        ///
        /// \param Motif The motif whose art is being authored.
        void DrawLeftPanelArt(Ref<Motif> Motif);

        /// \brief Draws the animation panel of the panel for the selected motif.
        ///
        /// \param Motif The motif whose animation is being displayed.
        void DrawLeftPanelAnimation(Ref<Motif> Motif);

        /// \brief Draws the right panel of the panel, showing a preview of the selected motif.
        ///
        /// \param Motif The motif to preview, shown as the slice of the sheet it was fired into.
        void DrawRightPanel(ConstRef<Motif> Motif);

        /// \brief Draws the bottom bar of the panel.
        void DrawStatusBar();

    private:

        /// \brief Gives a motif the frames of an image, replacing whatever art it held.
        ///
        /// \param Folder The project folder the sheets are rooted at.
        /// \param Motif  The motif taking the art.
        /// \param Sheet  The url of the sheet the frames are fired into, which the project authored.
        /// \param Source The image the frames are cut from, laid out left to right from the offset.
        /// \param Frames The number of frames to take from the source.
        /// \param Origin The corner of the first frame on the source, in texels.
        /// \param Width  The width of a frame, in texels.
        /// \param Height The height of a frame, in texels.
        /// \return `true` if every sheet was rebuilt, `false` otherwise.
        Bool Adopt(
            Text       Folder,
            UInt16     Motif,
            Text       Sheet,
            Text       Source,
            UInt16     Frames,
            IntVector2 Origin,
            UInt16     Width,
            UInt16     Height);

        /// \brief Takes a motif's art away, closing the gap its run leaves behind.
        ///
        /// \param Folder The project folder the sheets are rooted at.
        /// \param Motif  The motif losing its art.
        /// \return `true` if every sheet was rebuilt, `false` otherwise.
        Bool Discard(Text Folder, UInt16 Motif);

        /// \brief Rebuilds every sheet with one motif's art changed, then rebinds every run.
        ///
        /// \param Folder The project folder the sheets are rooted at.
        /// \param Change The art the motif takes on, whose source may be empty to take it away.
        /// \return `true` if every sheet was rebuilt, `false` otherwise.
        Bool Fire(Text Folder, ConstRef<Change> Change);

        /// \brief Gathers every motif's frames into the sheet it belongs to, assigning the runs as it goes.
        ///
        /// \param Folder     The project folder the sheets are rooted at.
        /// \param Change     The art one motif takes on, which replaces whatever it held.
        /// \param Batches    Receives one batch per sheet, in sheet order.
        /// \param Placements Receives the run every motif ends up with.
        void Gather(
            Text                              Folder,
            ConstRef<Change>                  Change,
            Ref<Sequence<Batch>>              Batches,
            Ref<Sequence<Tileset::Placement>> Placements) const;

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Repository>          mRepository;
        Ref<Tileset>             mTileset;
        Ref<Job::Service>        mScheduler;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UInt16                   mSelection;
        UInt16                   mScroll;
        Previewer                mPreview;
        Previewer                mInspector;
        Browser                  mBrowser;

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UInt16                   mAuthored;
        Str                      mSource;
        Bool                     mMeasured;
        UInt16                   mOriginX;
        UInt16                   mOriginY;
        UInt16                   mWidth;
        UInt16                   mHeight;
        Retainer<Graphic::Image> mArt;
        Str                      mFault;
    };
}
