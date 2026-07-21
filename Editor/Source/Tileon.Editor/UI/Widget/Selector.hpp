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

#include "Browser.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::UI
{
    /// \brief Shares one content browser between any number of inline asset fields.
    class Selector final
    {
    public:

        /// \brief Constructs the selector with the specified content service.
        ///
        /// \param Service The content service supplying the assets that can be selected.
        Selector(Ref<Content::Service> Service);

        /// \brief Opens the browser on behalf of the field identified by the given key.
        ///
        /// \param Key    The key identifying the requesting field.
        /// \param Filter The extension filter limiting which assets are offered.
        void Open(UInt64 Key, Text Filter);

        /// \brief Draws the shared browser, which must be called once per frame at window scope.
        ///
        /// \param Composer The UI composer used to render the browser.
        void Draw(Ref<Composer> Composer);

        /// \brief Claims the selection made on behalf of the field identified by the given key.
        ///
        /// \param Key       The key identifying the field claiming the selection.
        /// \param Selection The url of the selected asset, assigned only when a selection was pending.
        /// \return `true` if a selection was pending for the key, `false` otherwise.
        Bool Consume(UInt64 Key, Ref<Str> Selection);

        /// \brief Checks whether the browser is currently open on behalf of the given key.
        ///
        /// \param Key The key identifying the field to test.
        /// \return `true` if the field is awaiting a selection, `false` otherwise.
        ZY_INLINE Bool IsPending(UInt64 Key) const
        {
            return Key != 0 && mRequest == Key;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UI::Browser mBrowser;
        UInt64      mRequest;
        UInt64      mResult;
        Str         mSelection;
    };
}
