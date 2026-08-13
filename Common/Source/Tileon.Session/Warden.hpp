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

#include "Session.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a session that keeps the world for its watchers, with no camera and nothing to draw.
    class Warden final : public Session
    {
    public:

        /// \brief Largest radius, in regions, a single watcher may claim around itself.
        static constexpr UInt8 kMaxRadius = 16;

    public:

        /// \brief Constructs a warden instance with the specified service host.
        ///
        /// \param Host The service host to associate with the warden.
        explicit Warden(Ref<Engine::Subsystem::Host> Host);

        /// \brief Claims the square of regions a watcher needs, replacing whatever it claimed before.
        ///
        /// \param Watcher The identifier of the watcher, which is opaque to the warden.
        /// \param Region  The region the watcher sits in.
        /// \param Radius  The number of regions the watcher claims around itself, clamped to \ref kMaximumRadius.
        void Watch(UInt64 Watcher, IntVector2 Region, UInt8 Radius);

        /// \brief Drops a watcher's claim, freeing the regions no other watcher holds.
        ///
        /// \param Watcher The identifier of the watcher to drop.
        void Unwatch(UInt64 Watcher);

        /// \brief Applies the union of every claim to the world, loading and evicting regions to match.
        ///
        /// \return `true` if the resident set changed, `false` otherwise.
        Bool Tick();

    private:

        /// \brief Represents the square of regions a single watcher holds resident.
        struct Claim final
        {
            /// \brief The region the watcher sits in.
            IntVector2 Region;

            /// \brief The number of regions the watcher claims around itself.
            UInt8      Radius;

            /// \brief Default constructor.
            ZY_INLINE Claim()
                : Region { IntVector2() },
                  Radius { 0 }
            {
            }

            /// \brief Constructs a claim over the given square.
            ///
            /// \param Region The region the watcher sits in.
            /// \param Radius The number of regions the watcher claims around itself.
            ZY_INLINE Claim(IntVector2 Region, UInt8 Radius)
                : Region { Region },
                  Radius { Radius }
            {
            }
        };

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Table<UInt64, Claim> mWatchers;
        Bool                 mStale;
        Sequence<IntVector2> mResident;
    };
}