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

#include <Zyphryon.Content/Service.hpp>
#include <Zyphryon.Engine/Locator.hpp>
#include <Zyphryon.Graphic/Material.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents the art every terrain draws from, and the name each of its slices was authored under.
    class Splatset final : public Engine::Locator<Job::Service, Content::Service>
    {
    public:

        /// \brief The maximum number of terrains a project can author.
        static constexpr UInt16 kLimit    = 512;

        /// \brief The file the name of every terrain a project authored is kept in.
        static constexpr Text   kFilename = "Resources://Data/Splatset.bin";

        /// \brief The material holding the arrays every terrain is a slice of.
        static constexpr Text   kMaterial = "Resources://Material/Splatset/Splatset.mtl";

        /// \brief The array of base color every terrain draws a slice of, bound beside the material.
        static constexpr Text   kAlbedo   = "Resources://Material/Splatset/Splatset.tex";

        /// \brief The array of surface normals every terrain draws a slice of, bound beside the material.
        static constexpr Text   kNormal   = "Resources://Material/Splatset/Splatset_n.tex";

        /// \brief Represents one terrain, a slice of the splatset arrays dressed the way it was authored.
        struct Terrain final
        {
            /// The name the terrain is authored under, which nothing but the palette reads.
            Str       Name;

            /// How often the art repeats over the ground its own size would otherwise cover.
            Real32    Tiling = 1.0f;

            /// The color the art is multiplied by, so one slice can dress more than one terrain.
            IntColor8 Tint   = IntColor8::White();

            /// Whether the slice of the normal array this terrain draws was ever authored.
            Bool      Relief = false;

            /// \brief Serializes the state of the object to or from the specified archive.
            ///
            /// \param Archive The archive to serialize the object with.
            template<typename Serializer>
            ZY_INLINE void Serialize(Serializer Archive)
            {
                Archive.Serialize(Name);
                Archive.Serialize(Tiling);
                Archive.Serialize(Tint);
                Archive.Serialize(Relief);
            }
        };

    public:

        /// \brief Constructs a splatset with the specified service host.
        ///
        /// \param Host The service host to associate with the splatset.
        explicit Splatset(Ref<Engine::Subsystem::Host> Host);

        /// \brief Loads the name of every terrain from the splatset file.
        ///
        /// \note Completes asynchronously, no terrain is visible until the file and the material have loaded.
        void Load();

        /// \brief Saves the name of every terrain to the splatset file.
        void Save();

        /// \brief Adds a terrain naming the slice a bake has just appended to the arrays.
        ///
        /// \param Name The name to author the terrain under, which may be empty.
        /// \return The slice the new terrain draws.
        UInt16 AddTerrain(Text Name);

        /// \brief Forgets the terrain a slice draws, leaving its art in the arrays where it is.
        ///
        /// \param Slice The slice to forget.
        void RemoveTerrain(UInt16 Slice);

        /// \brief Gets the terrain a slice draws.
        ///
        /// \param Slice The slice to read.
        /// \return The terrain the slice draws.
        Ref<Terrain> GetTerrain(UInt16 Slice);

        /// \brief Iterates over every terrain a project authored.
        ///
        /// \param Callback The callback function to apply to each slice and the terrain that draws it.
        template<typename Function>
        ZY_INLINE void ForEachTerrain(AnyRef<Function> Callback)
        {
            Reconcile();

            for (UInt16 Slot = 1; Slot <= mRegistry.GetTop(); ++Slot)
            {
                if (mRegistry.IsOccupied(Slot))
                {
                    Callback(Slot - 1, mRegistry[mRegistry.GetKey(Slot)]);
                }
            }
        }

        /// \brief Gets the material every terrain is a slice of.
        ///
        /// \return The material the ground draws with.
        ZY_INLINE ConstRetainer<Graphic::Material> GetMaterial() const
        {
            return mMaterial;
        }

    private:

        /// \brief Names every slice the arrays hold that the splatset file did not account for.
        ///
        /// \note Does nothing until the file and the material have loaded, and runs only once thereafter.
        void Reconcile();

        /// \brief Loads the name of every terrain from the bytes of the splatset file.
        ///
        /// \param Result The result of reading the file.
        /// \param Data   The bytes the file held.
        void LoadDatabase(Filesystem::Result Result, AnyRef<Blob> Data);

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Pool<Terrain, kLimit, 0>    mRegistry;
        Retainer<Graphic::Material> mMaterial;
        Bool                        mLoaded;
        Bool                        mSeeded;
    };
}