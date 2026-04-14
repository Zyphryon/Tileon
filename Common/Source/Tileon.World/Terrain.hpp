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

namespace Tileon
{
    /// \brief Represents a terrain in the world, with properties and associated sprite.
    class Terrain final
    {
    public:

        /// \brief Defines the properties of the terrain.
        enum class Property : UInt16
        {
            Walkable  = 0b00000001,     ///< Indicates whether the terrain is walkable.
        };

    public:

        /// \brief Constructs a terrain with the specified unique identifier.
        ///
        /// \param ID The unique identifier for the terrain.
        ZYPHRYON_INLINE Terrain(UInt16 ID)
            : mID         { ID },
              mProperties { 0 }
        {
        }

        /// \brief Destructor for the Terrain class.
        ZYPHRYON_INLINE ~Terrain()
        {
            mID = 0;
        }

        /// \brief Gets the unique identifier of the terrain.
        ///
        /// \return The unique identifier of the terrain.
        ZYPHRYON_INLINE UInt16 GetID() const
        {
            return mID;
        }

        /// \brief Sets the name of the terrain.
        ///
        /// \param Name The name to assign to the terrain.
        ZYPHRYON_INLINE void SetName(ConstStr8 Name)
        {
            mName = Name;
        }

        /// \brief Gets the name of the terrain.
        ///
        /// \return The name of the terrain.
        ZYPHRYON_INLINE ConstStr8 GetName() const
        {
            return mName;
        }

        /// \brief Sets a property of the terrain.
        ///
        /// \param Mask   The property to set.
        /// \param Enable Whether to enable or disable the property.
        ZYPHRYON_INLINE void SetProperty(Property Mask, Bool Enable)
        {
            mProperties = SetOrClearBit(mProperties, Enum::Cast(Mask), Enable);
        }

        /// \brief Checks if the terrain has a specific property.
        ///
        /// \param Mask The property to check.
        /// \return `true` if the property is enabled, `false` otherwise.
        ZYPHRYON_INLINE Bool HasProperty(Property Mask) const
        {
            return HasBit(mProperties, Enum::Cast(Mask));
        }

        /// \brief Clones the properties, name, and sprite from another terrain.
        ///
        /// \param Terrain The terrain to clone from.
        ZYPHRYON_INLINE void Clone(ConstRef<Terrain> Terrain)
        {
            mProperties = Terrain.mProperties;
            mName       = Terrain.mName;
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
        {
            Archive.SerializeUInt(mID);
            Archive.SerializeUInt(mProperties);
            Archive.SerializeText(mName);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        UInt16 mID;
        UInt16 mProperties;
        Str8   mName;
    };
}
