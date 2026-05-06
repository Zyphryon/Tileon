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

namespace Tileon::Editor
{
    /// \brief Runtime property bag for the editor session, backed by a TOML document.
    class Session
    {
    public:

        /// \brief Sets a boolean value in the session with the specified key.
        ///
        /// \param Key   The key associated with the boolean value to set.
        /// \param Value The boolean value to set in the session.
        ZYPHRYON_INLINE void SetBool(ConstStr8 Key, Bool Value)
        {
            mData.GetRoot().SetBool(Key, Value);
        }

        /// \brief Gets a boolean value from the session with the specified key.
        ///
        /// \param Key     The key associated with the boolean value to retrieve.
        /// \param Default The default boolean value to return if the key does not exist.
        /// \return The boolean value associated with the specified key, or the default value if the key does not exist.
        ZYPHRYON_INLINE Bool GetBool(ConstStr8 Key, Bool Default = false) const
        {
            return mData.GetRoot().GetBool(Key, Default);
        }

        /// \brief Sets an enum value in the session with the specified key.
        ///
        /// \param Key   The key associated with the enum value to set.
        /// \param Value The enum value to set in the session.
        template<typename Type>
        ZYPHRYON_INLINE void SetEnum(ConstStr8 Key, Type Value)
            requires IsEnum<Type>
        {
            mData.GetRoot().SetEnum(Key, Value);
        }

        /// \brief Gets an enum value from the session with the specified key.
        ///
        /// \param Key     The key associated with the enum value to retrieve.
        /// \param Default The default enum value to return if the key does not exist.
        /// \return The enum value associated with the specified key, or the default value if the key does not exist.
        template<typename Type>
        ZYPHRYON_INLINE Type GetEnum(ConstStr8 Key, Type Default) const
            requires IsEnum<Type>
        {
            return mData.GetRoot().GetEnum(Key, Default);
        }

        /// \brief Sets an integer value in the session with the specified key.
        ///
        /// \param Key   The key associated with the integer value to set.
        /// \param Value The integer value to set in the session.
        ZYPHRYON_INLINE void SetInteger(ConstStr8 Key, SInt64 Value)
        {
            mData.GetRoot().SetInteger(Key, Value);
        }

        /// \brief Gets an integer value from the session with the specified key.
        ///
        /// \param Key     The key associated with the integer value to retrieve.
        /// \param Default The default integer value to return if the key does not exist.
        /// \return The integer value associated with the specified key, or the default value if the key does not exist.
        ZYPHRYON_INLINE SInt64 GetInteger(ConstStr8 Key, SInt64 Default = 0) const
        {
            return mData.GetRoot().GetInteger(Key, Default);
        }

        /// \brief Sets a real value in the session with the specified key.
        ///
        /// \param Key   The key associated with the real value to set.
        /// \param Value The real value to set in the session.
        ZYPHRYON_INLINE void SetReal(ConstStr8 Key, Real64 Value)
        {
            mData.GetRoot().SetReal(Key, Value);
        }

        /// \brief Gets a real value from the session with the specified key.
        ///
        /// \param Key     The key associated with the real value to retrieve.
        /// \param Default The default real value to return if the key does not exist.
        /// \return The real value associated with the specified key, or the default value if the key does not exist.
        ZYPHRYON_INLINE Real64 GetReal(ConstStr8 Key, Real64 Default = 0.0) const
        {
            return mData.GetRoot().GetReal(Key, Default);
        }

        /// \brief Sets a string value in the session with the specified key.
        ///
        /// \param Key   The key associated with the string value to set.
        /// \param Value The string value to set in the session.
        ZYPHRYON_INLINE void SetString(ConstStr8 Key, ConstStr8 Value)
        {
            mData.GetRoot().SetString(Key, Value);
        }

        /// \brief Gets a string value from the session with the specified key.
        ///
        /// \param Key     The key associated with the string value to retrieve.
        /// \param Default The default string value to return if the key does not exist.
        /// \return The string value associated with the specified key, or the default value if the key does not exist.
        ZYPHRYON_INLINE ConstStr8 GetString(ConstStr8 Key, ConstStr8 Default = "") const
        {
            return mData.GetRoot().GetString(Key, Default);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        mutable TOMLParser mData;
    };
}

