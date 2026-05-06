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

#include "Component/Sprite/Animator.hpp"
#include "Tileon.World/Repository.hpp"
#include <Zyphryon.Graphic/Material.hpp>
#include <Zyphryon.Math/Color.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a tileset that manages the rendering data for different terrains in the world.
    class Tileset final : public Locator<Content::Service>
    {
    public:

        /// \brief Maximum number of terrains that can be stored in the repository.
        static constexpr UInt32    kLimit    = Repository::kTerrainLimit;

        /// \brief Default filename for storing terrain data.
        static constexpr ConstStr8 kFilename = "Resources://Data/Tileset.bin";

        /// \brief Holds the rendering data for a single terrain type.
        struct Entry final
        {
            /// \brief The URI of the material used for rendering the terrain.
            Content::Uri                Path;

            /// \brief The number of tile columns the terrain sprite spans.
            UInt8                       Columns  { 1 };

            /// \brief The number of tile rows the terrain sprite spans.
            UInt8                       Rows     { 1 };

            /// \brief The current keyframe index for animating the terrain.
            UInt8                       Keyframe { 0 };

            /// \brief The tint color applied to the terrain when rendering.
            IntColor8                   Tint     { IntColor8::White() };

            /// \brief The material cache used for rendering the terrain.
            Tracker<Graphic::Material>  Material;

            /// \brief The animation sequence used to render the terrain.
            Animation                   Animation;

            /// \brief Checks if the entry is valid by verifying that the material path is valid.
            ///
            /// \return `true` if the entry is valid, `false` otherwise.
            ZYPHRYON_INLINE Bool IsValid() const
            {
                return Path.IsValid();
            }

            /// \brief Updates the keyframe index based on the current time and the animation sequence.
            ///
            /// \param Time The current time in seconds used to determine the keyframe index for animation.
            ZYPHRYON_INLINE void Tick(Real64 Time)
            {
                Keyframe = Animator::Sample<Animator::Status::Repeat>(Animation, Time);
            }

            /// \brief Serializes the state of the object to or from the specified archive.
            ///
            /// \param Archive The archive to serialize the object with.
            template<typename Serializer>
            ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
            {
                Archive.SerializeObject(Path);
                Archive.SerializeUInt8(Columns);
                Archive.SerializeUInt8(Rows);
                Archive.SerializeObject(Tint);
                Archive.SerializeObject(Animation);
            }
        };

    public:

        /// \brief Constructs a tileset with the specified service host.
        ///
        /// \param Host The service host to associate with the tileset.
        explicit Tileset(Ref<Service::Host> Host);

        /// \brief Loads the tile data from the tileset file.
        void Load();

        /// \brief Saves the tile data to the tileset file.
        void Save();

        /// \brief Updates the ticking entries in the tileset.
        void Tick(Real64 Time);

        /// \brief Preloads the materials for all valid entries in the tileset.
        void Preload();

        /// \brief Creates a new entry in the tileset for the specified terrain.
        ///
        /// \param Terrain The terrain for which to create the entry.
        /// \return The created entry for the terrain.
        Ref<Entry> CreateEntry(ConstRef<Terrain> Terrain);

        /// \brief Deletes an entry from the tileset for the specified terrain.
        ///
        /// \param Terrain The terrain for which to delete the entry.
        void DeleteEntry(ConstRef<Terrain> Terrain);

        /// \brief Checks if an entry with the given unique identifier exists in the repository.
        ///
        /// \param ID The unique identifier of the entry to check for existence.
        /// \return `true` if a entry with the given ID exists, `false`
        ZYPHRYON_INLINE Bool HasEntry(UInt16 ID) const
        {
            return mRegistry.IsAllocated(ID);
        }

        /// \brief Gets an entry from the tileset by its unique identifier.
        ///
        /// \param ID The unique identifier of the entry.
        /// \return A reference to the entry corresponding to the given ID.
        ZYPHRYON_INLINE Ref<Entry> GetEntry(UInt16 ID)
        {
            return mRegistry[ID];
        }

        /// \brief Gets an entry from the tileset by its unique identifier.
        ///
        /// \param ID The unique identifier of the entry.
        /// \return A constant reference to the entry corresponding to the given ID.
        ZYPHRYON_INLINE ConstRef<Entry> GetEntry(UInt16 ID) const
        {
            return mRegistry[ID];
        }

    private:

        /// \brief Loads the tileset database from file.
        ///
        /// \return `true` if the database was loaded successfully, `false` otherwise.
        Bool LoadDatabase();

        /// \brief Saves the tileset database to file.
        void SaveDatabase();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Catalog<Entry, kLimit> mRegistry;
    };
}