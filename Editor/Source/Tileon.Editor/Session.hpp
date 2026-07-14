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
    /// \brief Runtime property bag for the editor session, backed by a JSON document.
    class Session
    {
    public:

        /// \brief Constructs a new session object.
        Session();

        /// \brief Loads session data from a JSON string.
        ///
        /// \param Data A JSON string containing the session data to load.
        /// \return `true` if the session data was successfully loaded, `false` otherwise.
        Bool Load(Text Data);

        /// \brief Saves the session data to a JSON string.
        ///
        /// \return A JSON string representing the current session data.
        Str Save() const;

        /// \brief Sets a boolean value in the session with the specified key.
        ///
        /// \param Key   The key associated with the boolean value to set.
        /// \param Value The boolean value to set in the session.
        ZY_INLINE void SetBool(Text Key, Bool Value)
        {
            mObject.SetBool(Key, Value);
        }

        /// \brief Gets a boolean value from the session with the specified key.
        ///
        /// \param Key     The key associated with the boolean value to retrieve.
        /// \param Default The default boolean value to return if the key does not exist.
        /// \return The boolean value associated with the specified key, or the default value if the key does not exist.
        ZY_INLINE Bool GetBool(Text Key, Bool Default = false) const
        {
            return mObject.GetBool(Key, Default);
        }

        /// \brief Sets an enum value in the session with the specified key.
        ///
        /// \param Key   The key associated with the enum value to set.
        /// \param Value The enum value to set in the session.
        template<typename Type>
        ZY_INLINE void SetEnum(Text Key, Type Value)
            requires IsEnum<Type>
        {
            mObject.SetEnum(Key, Value);
        }

        /// \brief Gets an enum value from the session with the specified key.
        ///
        /// \param Key     The key associated with the enum value to retrieve.
        /// \param Default The default enum value to return if the key does not exist.
        /// \return The enum value associated with the specified key, or the default value if the key does not exist.
        template<typename Type>
        ZY_INLINE Type GetEnum(Text Key, Type Default) const
            requires IsEnum<Type>
        {
            return mObject.GetEnum(Key, Default);
        }

        /// \brief Sets an integer value in the session with the specified key.
        ///
        /// \param Key   The key associated with the integer value to set.
        /// \param Value The integer value to set in the session.
        ZY_INLINE void SetInteger(Text Key, SInt64 Value)
        {
            mObject.SetNumber(Key, Value);
        }

        /// \brief Gets an integer value from the session with the specified key.
        ///
        /// \param Key     The key associated with the integer value to retrieve.
        /// \param Default The default integer value to return if the key does not exist.
        /// \return The integer value associated with the specified key, or the default value if the key does not exist.
        ZY_INLINE SInt64 GetInteger(Text Key, SInt64 Default = 0) const
        {
            return mObject.GetNumber(Key, Default);
        }

        /// \brief Sets a real value in the session with the specified key.
        ///
        /// \param Key   The key associated with the real value to set.
        /// \param Value The real value to set in the session.
        ZY_INLINE void SetReal(Text Key, Real64 Value)
        {
            mObject.SetNumber(Key, Value);
        }

        /// \brief Gets a real value from the session with the specified key.
        ///
        /// \param Key     The key associated with the real value to retrieve.
        /// \param Default The default real value to return if the key does not exist.
        /// \return The real value associated with the specified key, or the default value if the key does not exist.
        ZY_INLINE Real64 GetReal(Text Key, Real64 Default = 0.0) const
        {
            return mObject.GetNumber(Key, Default);
        }

        /// \brief Sets a string value in the session with the specified key.
        ///
        /// \param Key   The key associated with the string value to set.
        /// \param Value The string value to set in the session.
        ZY_INLINE void SetString(Text Key, Text Value)
        {
            mObject.SetString(Key, Value);
        }

        /// \brief Gets a string value from the session with the specified key.
        ///
        /// \param Key     The key associated with the string value to retrieve.
        /// \param Default The default string value to return if the key does not exist.
        /// \return The string value associated with the specified key, or the default value if the key does not exist.
        ZY_INLINE Text GetString(Text Key, Text Default = "") const
        {
            return mObject.GetString(Key, Default);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        mutable JsonValue  mDocument;
        mutable JsonObject mObject;
    };
}

