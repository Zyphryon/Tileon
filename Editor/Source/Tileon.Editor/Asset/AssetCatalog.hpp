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

#include "AssetType.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    /// \brief Publishes the kinds of file the editor knows how to list, reread and describe.
    class AssetCatalog final
    {
    public:

        /// \brief Constructs a catalogue holding every kind the editor knows.
        AssetCatalog();

        /// \brief Gets the kind a file name belongs to.
        ///
        /// \param Name The file name to classify, with or without its folder.
        /// \return The kind's descriptor, or `nullptr` when the editor has nothing to say about the file.
        ConstPtr<AssetType> Find(Text Name) const;

        /// \brief Gets every published kind, in registration order.
        ///
        /// \return The catalogue of kinds.
        ZY_INLINE ConstSpan<AssetType> GetTypes() const
        {
            return mTypes;
        }

    private:

        /// \brief Publishes a kind the content service knows how to load.
        ///
        /// \tparam Type      The resource type the kind loads as.
        /// \param  Extension The suffix the kind is recognised by, including its dot.
        /// \param  Label     The human-readable name displayed for the kind.
        /// \param  Icon      The glyph displayed alongside the label.
        /// \param  Traits    What the editor can do with the kind, beyond listing it.
        template<typename Type>
        ZY_INLINE void Add(Text Extension, Text Label, Text Icon, AssetType::Trait Traits = AssetType::Trait::None)
        {
            mTypes.Append(AssetType::Create<Type>(Extension, Label, Icon, Traits));
        }

        /// \brief Publishes a kind nothing loads, such as the art an import bakes from.
        ///
        /// \param Extension The suffix the kind is recognised by, including its dot.
        /// \param Label     The human-readable name displayed for the kind.
        /// \param Icon      The glyph displayed alongside the label.
        ZY_INLINE void Add(Text Extension, Text Label, Text Icon)
        {
            mTypes.Append(AssetType::Create(Extension, Label, Icon));
        }

        /// \brief Publishes every kind of file the editor knows about.
        void OnRegister();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Sequence<AssetType> mTypes;
    };
}