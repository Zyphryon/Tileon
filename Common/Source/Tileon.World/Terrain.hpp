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
    /// \brief Represents a terrain type in the world, defining its properties and characteristics.
    class Terrain final
    {
    public:

        /// \brief Enumerates the properties that can be assigned to a terrain type.
        enum class Property : UInt16
        {
            Walkable  = 0b00000001,     ///< Indicates whether the terrain is walkable.
        };

    public:

        /// \brief Constructs an empty terrain.
        ZY_INLINE Terrain()
            : mID         { 0 },
              mProperties { 0 }
        {
        }

        /// \brief Constructs a terrain with the specified unique identifier.
        ///
        /// \param ID The unique identifier for the terrain.
        ZY_INLINE Terrain(UInt16 ID)
            : mID         { ID },
              mProperties { 0 }
        {
        }

        /// \brief Constructs a terrain by copying the properties of another terrain, but with a new unique identifier.
        ///
        /// \param ID   The unique identifier for the new terrain.
        /// \param Copy The terrain to copy the properties from.
        ZY_INLINE Terrain(UInt16 ID, ConstRef<Terrain> Copy)
            : mID         { ID },
              mProperties { Copy.mProperties },
              mName       { Copy.mName }
        {
        }

        /// \brief Destructor for the Terrain class.
        ZY_INLINE ~Terrain()
        {
            mID = 0;
        }

        /// \brief Gets the unique identifier of the terrain.
        ///
        /// \return The unique identifier of the terrain.
        ZY_INLINE UInt16 GetID() const
        {
            return mID;
        }

        /// \brief Sets the name of the terrain.
        ///
        /// \param Name The name to assign to the terrain.
        ZY_INLINE void SetName(Text Name)
        {
            mName = Name;
        }

        /// \brief Gets the name of the terrain.
        ///
        /// \return The name of the terrain.
        ZY_INLINE Text GetName() const
        {
            return mName;
        }

        /// \brief Changes the state of a specific property for the terrain.
        ///
        /// \param Mask   The property to set or clear.
        /// \param Enable `true` to set the property, `false` to clear it.
        ZY_INLINE void SetProperty(Property Mask, Bool Enable)
        {
            mProperties = SetOrClearBit(mProperties, Enum::Cast(Mask), Enable);
        }

        /// \brief Checks if the terrain has a specific property.
        ///
        /// \param Mask The property to check.
        /// \return `true` if the property is enabled, `false` otherwise.
        ZY_INLINE Bool HasProperty(Property Mask) const
        {
            return HasBit(mProperties, Enum::Cast(Mask));
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZY_INLINE void Serialize(Serializer Archive)
        {
            Archive.Serialize(mID);
            Archive.Serialize(mProperties);
            Archive.Serialize(mName);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Str    mName;
        UInt16 mID;
        UInt16 mProperties;
    };
}
