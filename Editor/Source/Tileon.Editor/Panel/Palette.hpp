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
#include "Tileon.Editor/Panel/Viewport/Tools.hpp"
#include "Tileon.Editor/Toolkit/Gallery.hpp"
#include "Tileon.Render/Terrain/Splatset.hpp"
#include "Tileon.World/Repository.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Provides a palette interface for browsing and selecting terrains or archetypes in the editor.
    class Palette final : public Panel
    {
    public:

        /// \brief Constructs the activity with the specified context.
        ///
        /// \param Context The context associated with this activity.
        Palette(Ref<Context> Context);

        /// \see Panel::OnDraw()
        void OnDraw() override;

        /// \brief Writes the arrays back out with every slice added since they were last assembled.
        void OnCommit() override;

    private:

        /// \brief The number of slices a bake leaves room for beyond the terrains it holds.
        static constexpr UInt16 kHeadroom = 8;

        /// \brief Draws the tab listing the terrains the ground can be painted with.
        void DrawTerrainTab();

        /// \brief Draws the fields that add a terrain to the tileset.
        void DrawTerrainAuthor();

        /// \brief Adds one terrain to the tileset, as a slice appended to each of its arrays.
        ///
        /// \param Albedo The art the terrain is coloured by.
        /// \param Normal The art the terrain takes its relief from, which may be empty.
        /// \param Name   The name to author the terrain under, which may be empty.
        /// \return `true` when the terrain was added, `false` when the bake was refused or failed.
        Bool AppendTerrain(Text Albedo, Text Normal, Text Name);

        /// \brief Bakes one terrain's art straight into the arrays the splatset already holds.
        ///
        /// \param Albedo The resolved path of the terrain's base color.
        /// \param Normal The resolved path of the terrain's surface normals, which may be empty.
        /// \param Name   The name to author the terrain under.
        /// \return `true` when the terrain was written, `false` when its art could not be read or matched.
        Bool AppendSlice(Text Albedo, Text Normal, Text Name);

        /// \brief Draws the gallery of terrains available to paint.
        /// \brief Draws the editable properties of the selected terrain.
        void DrawTerrainProperties();

        /// \brief Counts the terrains the splatset holds.
        ///
        /// \return The number of slices in use, which is where the next terrain goes.
        UInt16 CountTerrains();

        /// \brief Writes flat relief into one slice, for a terrain nobody gave a normal map.
        ///
        /// \param Array The array of surface normals to write into.
        /// \param Slice The slice to make flat.
        void WriteFlatRelief(ConstRetainer<Graphic::Image> Array, UInt16 Slice);

        /// \brief Writes every level of one slice of an array the material holds.
        ///
        /// \param Array  The array to write into.
        /// \param Slice  The slice to write.
        /// \param Pixels The levels of the slice, one after another, in the format the array holds.
        void WriteSlice(ConstRetainer<Graphic::Image> Array, UInt16 Slice, ConstSpan<Byte> Pixels);

        /// \brief Resolves a mounted path into the one on disk the baker reads and writes.
        ///
        /// \param Path The path to resolve, which may already name a plain one.
        /// \return The path on disk.
        Str Resolve(Text Path);

        /// \brief Bakes a normal map into the slice a terrain already occupies.
        ///
        /// \param Slice  The slice to write, which the terrain already draws its colour from.
        /// \param Source The path of the art to take the relief from.
        /// \return `true` when the relief was written, `false` when the art could not be read or matched.
        Bool AppendRelief(UInt16 Slice, Text Source);

        void DrawTerrainGallery();

        /// \brief Draws the status bar contents describing the selected terrain.
        void DrawTerrainStatus();

        /// \brief Draws the tab listing the archetypes available in the repository.
        void DrawEntityTab();

        /// \brief Draws the gallery of archetypes available in the repository.
        void DrawEntityGallery();

        /// \brief Draws the status bar contents describing the selected archetype.
        void DrawEntityStatus();

        /// \brief Holds the art of one terrain added since the arrays were last written out.
        struct Pending final
        {
            /// The resolved path of the terrain's base color.
            Str Albedo;

            /// The resolved path of the terrain's surface normals, which may be empty.
            Str Normal;
        };

        /// \brief Draws a centered, dimmed hint inside the status bar.
        ///
        /// \param Hint The message to display.
        void DrawHint(Text Hint);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Ref<Repository>    mRepository;
        Ref<Splatset>      mSplatset;
        Toolkit::Gallery   mTerrains;
        Toolkit::Gallery   mEntities;
        SInt32             mMode;
        Str                mPendingAlbedo;
        Str                mPendingNormal;
        Str                mPendingName;
        Sequence<Pending>  mPending;
        Table<UInt16, Str> mReliefs;
    };
}