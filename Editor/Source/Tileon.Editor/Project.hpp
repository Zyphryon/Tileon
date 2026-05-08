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
    /// \brief Represets a project, which encapsulates the metadata and configuration for a game project.
    class Project final
    {
    public:

        /// \brief The expected file description for project files.
        static constexpr auto kFileDescription = "Tileon Project File";

        /// \brief The expected file extension for project files.
        static constexpr auto kFileExtension   = "tileon";

    public:

        /// \brief Initializes the project with default values.
        Project();

        /// \brief Loads the project data from the specified TOML archive.
        ///
        /// \param Archive The TOML parser containing the project data to load.
        /// \return `true` if the project was loaded successfully, `false` otherwise.
        Bool Load(Ref<TOMLParser> Archive);

        /// \brief Saves the project data to the specified TOML archive.
        ///
        /// \param Archive The TOML parser to save the project data into.
        void Save(Ref<TOMLParser> Archive) const;

        /// \brief Sets the file path of the project.
        ///
        /// \param Path The file path to assign to the project.
        ZYPHRYON_INLINE void SetPath(AnyRef<Str8> Path)
        {
            mPath = Move(Path);
        }

        /// \brief Gets the file path of the project.
        ///
        /// \return The file path of the project.
        ZYPHRYON_INLINE ConstStr8 GetPath() const
        {
            return mPath;
        }

        /// \brief Sets the display name of the project.
        ///
        /// \param Name The display name to assign to the project.
        ZYPHRYON_INLINE void SetName(ConstStr8 Name)
        {
            mName = Name;
        }

        /// \brief Gets the display name of the project.
        ///
        /// \return The display name of the project.
        ZYPHRYON_INLINE ConstStr8 GetName() const
        {
            return mName;
        }

        /// \brief Sets the author of the project.
        ///
        /// \param Author The author name to assign to the project.
        ZYPHRYON_INLINE void SetAuthor(ConstStr8 Author)
        {
            mAuthor = Author;
        }

        /// \brief Gets the author of the project.
        ///
        /// \return The author name of the project.
        ZYPHRYON_INLINE ConstStr8 GetAuthor() const
        {
            return mAuthor;
        }

        /// \brief Sets the description of the project.
        ///
        /// \param Description The description to assign to the project.
        ZYPHRYON_INLINE void SetDescription(ConstStr8 Description)
        {
            mDescription = Description;
        }

        /// \brief Gets the description of the project.
        ///
        /// \return The description of the project.
        ZYPHRYON_INLINE ConstStr8 GetDescription() const
        {
            return mDescription;
        }

        /// \brief Sets the tile density (pixels per world unit) for the project.
        ///
        /// \param Density The density value to assign (e.g. 32 pixels per tile).
        ZYPHRYON_INLINE void SetDensity(UInt16 Density)
        {
            mDensity = Density;
        }

        /// \brief Gets the tile density (pixels per world unit) of the project.
        ///
        /// \return The density value of the project.
        ZYPHRYON_INLINE UInt16 GetDensity() const
        {
            return mDensity;
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        Str8   mPath;
        Str8   mName;
        Str8   mAuthor;
        Str8   mDescription;
        UInt16 mDensity;
    };
}

