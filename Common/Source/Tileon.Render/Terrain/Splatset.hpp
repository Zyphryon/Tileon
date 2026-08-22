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
#include "Tileon.Render/Types.hpp"

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
        static constexpr UInt16 kLimit    = 1'024;

        /// \brief The slice handed back when a terrain could not be added.
        static constexpr UInt16 kInvalid  = 0xFFFF;

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
            Str       Name    = "Unnamed";

            /// How often the art repeats over the ground its own size would otherwise cover.
            Real32    Tiling  = 1.0f;

            /// How wide a band the relief feathers over where the terrain meets another.
            Real32    Feather = 0.2f;

            /// The color the art is multiplied by, so one slice can dress more than one terrain.
            IntColor8 Tint    = IntColor8::White();

            /// The art the colour of the slice was baked from.
            Str       Albedo;

            /// The art the relief of the slice was baked from, empty when the terrain brought none.
            Str       Normal;

            /// The art the elevation folded into the colour was baked from, empty when it brought none.
            Str       Height;

            /// Whether the slice has been retired.
            Bool      Retired = false;

            /// \brief Serializes the state of the object to or from the specified archive.
            ///
            /// \param Archive The archive to serialize the object with.
            template<typename Serializer>
            ZY_INLINE void Serialize(Serializer Archive)
            {
                Archive.Serialize(Name);
                Archive.Serialize(Tiling);
                Archive.Serialize(Feather);
                Archive.Serialize(Tint);
                Archive.Serialize(Albedo);
                Archive.Serialize(Normal);
                Archive.Serialize(Height);
                Archive.Serialize(Retired);
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
        /// \return The slice the new terrain draws, or #kInvalid when the arrays are full.
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

        /// \brief Gets every slice the arrays hold, retired or not, indexed by the slice it draws.
        ///
        /// \return The terrains of the project.
        ZY_INLINE ConstSpan<Terrain> GetTerrains()
        {
            Reconcile();

            return mRegistry;
        }

        /// \brief Sets the extent every slice of the arrays is baked at.
        ///
        /// \param Resolution The extent, in texels.
        ZY_INLINE void SetResolution(UInt16 Resolution)
        {
            mResolution = Resolution;
        }

        /// \brief Gets the extent every slice of the arrays is baked at.
        ///
        /// \return The extent, in texels.
        ZY_INLINE UInt16 GetResolution() const
        {
            return mResolution;
        }

        /// \brief Gets the material every terrain is a slice of.
        ///
        /// \return The material the ground draws with.
        ZY_INLINE ConstRetainer<Graphic::Material> GetMaterial() const
        {
            return mMaterial;
        }

        /// \brief Serializes every terrain a project authored to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the splatset with.
        template<typename Serializer>
        ZY_INLINE void Serialize(Serializer Archive)
        {
            UInt32 Magic   = 0x4C505354;
            UInt32 Version = 0x01;

            Archive.Serialize(Magic);
            Archive.Serialize(Version);

            if constexpr (Serializer::IsReader)
            {
                if (Magic != 0x4C505354 || Version != 0x01)
                {
                    LOG_W("Splatset: '{0}' is not written in a layout this build reads", kFilename);
                    return;
                }
            }

            Archive.Serialize(mRegistry);
            Archive.Serialize(mResolution);
        }

    private:

        /// \brief Names every slice the arrays hold that the splatset file did not account for.
        ///
        /// \note Does nothing until the file and the material have loaded, and runs only once thereafter.
        void Reconcile();

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Sequence<Terrain, kLimit>   mRegistry;
        Retainer<Graphic::Material> mMaterial;
        UInt16                      mResolution;
        Bool                        mLoaded;
        Bool                        mSeeded;
    };
}