// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#pragma once

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Toolkit
{
    /// \brief Walks the suffixes a filter names, handing each to the given action.
    ///
    /// \param Filter The suffixes, separated by spaces.
    /// \param Action The action invoked with each suffix, which stops the walk by answering `false`.
    template<typename Callback>
    constexpr void ForEachSuffix(Text Filter, AnyRef<Callback> Action)
    {
        for (UInt Cursor = 0; Cursor < Filter.GetSize(); )
        {
            const SInt Break  = StrFind(Filter.Slice(Cursor), ' ');
            const UInt Length = (Break < 0 ? Filter.GetSize() - Cursor : static_cast<UInt>(Break));

            if (Length > 0 && !Action(Filter.Slice(Cursor, Length)))
            {
                return;
            }
            Cursor += Length + 1;
        }
    }

    /// \brief Checks whether a name ends in one of the suffixes a filter names.
    ///
    /// \param Filter The suffixes to accept, separated by spaces, or empty to accept everything.
    /// \param Name   The file name to test.
    /// \return `true` when the name carries one of the suffixes, `false` otherwise.
    constexpr Bool HasSuffix(Text Filter, Text Name)
    {
        if (Filter.IsEmpty())
        {
            return true;
        }

        Bool Accepted = false;

        ForEachSuffix(Filter, [&](Text Suffix)
        {
            Accepted = StrEndsWith(Name, Suffix);
            return !Accepted;
        });
        return Accepted;
    }
}