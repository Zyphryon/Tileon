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

#include "Region.hpp"
#include <Zyphryon.Math/Scalar.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a placement in the world, defined by a region and an offset within that region.
    class Placement final
    {
    public:

        /// \brief The minimum region coordinate in the world.
        static constexpr SInt16 kMinRegion = kMinimum<SInt16>;

        /// \brief The maximum region coordinate in the world.
        static constexpr SInt16 kMaxRegion = kMaximum<SInt16>;

        /// \brief The minimum tile coordinate in the world per x-axis.
        static constexpr SInt32 kMinTileX  = static_cast<SInt32>(kMinRegion) * Region::kTilesPerX;

        /// \brief The maximum tile coordinate in the world per x-axis.
        static constexpr SInt32 kMaxTileX  = (static_cast<SInt32>(kMaxRegion) + 1) * Region::kTilesPerX - 1;

        /// \brief The minimum tile coordinate in the world per y-axis.
        static constexpr SInt32 kMinTileY  = static_cast<SInt32>(kMinRegion) * Region::kTilesPerY;

        /// \brief The maximum tile coordinate in the world per y-axis.
        static constexpr SInt32 kMaxTileY  = (static_cast<SInt32>(kMaxRegion) + 1) * Region::kTilesPerY - 1;

    public:

        /// \brief Default constructor initializing the placement at the origin (0, 0) with no offset.
        ZYPHRYON_INLINE constexpr Placement()
            : mRegionX { 0 },
              mRegionY { 0 },
              mOffsetX { 0.0f },
              mOffsetY { 0.0f }
        {
        }

        /// \brief Constructs a placement with the specified region coordinates and offset within that region.
        ///
        /// \param RegionX The x-coordinate of the region.
        /// \param RegionY The y-coordinate of the region.
        /// \param OffsetX The x-offset within the region, representing the placement relative to the region's origin.
        /// \param OffsetY The y-offset within the region, representing the placement relative to the region's origin.
        ZYPHRYON_INLINE constexpr Placement(SInt16 RegionX, SInt16 RegionY, Real32 OffsetX, Real32 OffsetY)
            : mRegionX { RegionX },
              mRegionY { RegionY },
              mOffsetX { OffsetX },
              mOffsetY { OffsetY }
        {
        }

        /// \brief Gets the x-coordinate of the region that this placement belongs to.
        ///
        /// \return The x-coordinate of the region.
        ZYPHRYON_INLINE constexpr SInt16 GetRegionX() const
        {
            return mRegionX;
        }

        /// \brief Gets the y-coordinate of the region that this placement belongs to.
        ///
        /// \return The y-coordinate of the region.
        ZYPHRYON_INLINE constexpr SInt16 GetRegionY() const
        {
            return mRegionY;
        }

        /// \brief Gets the x-offset within the region, representing the placement relative to the region's origin.
        ///
        /// \return The x-offset within the region.
        ZYPHRYON_INLINE constexpr Real32 GetOffsetX() const
        {
            return mOffsetX;
        }

        /// \brief Gets the y-offset within the region, representing the placement relative to the region's origin.
        ///
        /// \return The y-offset within the region.
        ZYPHRYON_INLINE constexpr Real32 GetOffsetY() const
        {
            return mOffsetY;
        }

        /// \brief Gets the shifted x-coordinate of the region.
        ///
        /// \return The shifted x-coordinate of the region.
        ZYPHRYON_INLINE constexpr SInt32 GetBaseX() const
        {
            return mRegionX * Region::kTilesPerX;
        }

        /// \brief Gets the shifted y-coordinate of the region.
        ///
        /// \return The shifted y-coordinate of the region.
        ZYPHRYON_INLINE constexpr SInt32 GetBaseY() const
        {
            return mRegionY * Region::kTilesPerY;
        }

        /// \brief Gets the absolute x-coordinate in the world by combining the region's x-coordinate and the offset.
        ///
        /// \return The absolute x-coordinate in the world.
        ZYPHRYON_INLINE constexpr Real64 GetAbsoluteX() const
        {
            return (mRegionX * Region::kTilesPerX) + mOffsetX;
        }

        /// \brief Gets the absolute y-coordinate in the world by combining the region's y-coordinate and the offset.
        ///
        /// \return The absolute y-coordinate in the world.
        ZYPHRYON_INLINE constexpr Real64 GetAbsoluteY() const
        {
            return (mRegionY * Region::kTilesPerY) + mOffsetY;
        }

        /// \brief Equality operator to compare two placements for equivalence.
        ///
        /// \param Other The other placement to compare against.
        /// \return `true` if the placements are equal, `false` otherwise.
        ZYPHRYON_INLINE constexpr Bool operator==(ConstRef<Placement> Other) const
        {
            return mRegionX == Other.mRegionX && mRegionY == Other.mRegionY
                && mOffsetX == Other.mOffsetX && mOffsetY == Other.mOffsetY;
        }

        /// \brief Inequality operator to compare two placements for non-equivalence.
        ///
        /// \param Other The other placement to compare against.
        /// \return `true` if the placements are not equal, `false` otherwise.
        ZYPHRYON_INLINE constexpr Bool operator!=(ConstRef<Placement> Other) const
        {
            return !(*this == Other);
        }

        /// \brief Serializes the state of the object to or from the specified archive.
        ///
        /// \param Archive The archive to serialize the object with.
        template<typename Serializer>
        ZYPHRYON_INLINE void OnSerialize(Serializer Archive)
        {
            Archive.SerializeInt(mRegionX);
            Archive.SerializeInt(mRegionY);
            Archive.SerializeReal32(mOffsetX);
            Archive.SerializeReal32(mOffsetY);
        }

    public:

        /// \brief Creates a placement from the specified region coordinates and offset.
        ///
        /// \param RegionX The x-coordinate of the region.
        /// \param RegionY The y-coordinate of the region.
        ZYPHRYON_INLINE static constexpr Placement FromRegion(SInt16 RegionX, SInt16 RegionY)
        {
            return Placement(RegionX, RegionY, 0.0f, 0.0f);
        }

        /// \brief Creates a placement from absolute coordinates, calculating the corresponding region and offset.
        ///
        /// \param AbsoluteX The absolute x-coordinate in the world, which will be used to determine the region and offset.
        /// \param AbsoluteY The absolute y-coordinate in the world, which will be used to determine the region and offset.
        ZYPHRYON_INLINE static constexpr Placement FromAbsolute(Real64 AbsoluteX, Real64 AbsoluteY)
        {
            const SInt16 RegionX = static_cast<SInt16>(Floor(AbsoluteX / (Region::kTilesPerX)));
            const SInt16 RegionY = static_cast<SInt16>(Floor(AbsoluteY / (Region::kTilesPerY)));
            const Real32 OffsetX = static_cast<Real32>(AbsoluteX - (RegionX * Region::kTilesPerX));
            const Real32 OffsetY = static_cast<Real32>(AbsoluteY - (RegionY * Region::kTilesPerY));
            return Placement(RegionX, RegionY, OffsetX, OffsetY);
        }

        /// \brief Normalizes the placement by adjusting the region coordinates and offset.
        ///
        /// \param Input The placement to normalize.
        /// \return A new placement with normalized region coordinates and offset.
        ZYPHRYON_INLINE static constexpr Placement Normalize(Placement Input)
        {
            const SInt32 DeltaX = static_cast<SInt32>(Floor(Input.GetOffsetX() / Region::kTilesPerX));
            const SInt32 DeltaY = static_cast<SInt32>(Floor(Input.GetOffsetY() / Region::kTilesPerY));

            return Placement(
                Input.GetRegionX() + DeltaX,
                Input.GetRegionY() + DeltaY,
                Input.GetOffsetX() - DeltaX * Region::kTilesPerX,
                Input.GetOffsetY() - DeltaY * Region::kTilesPerY);
        }

        /// \brief Clamps a placement to the valid world boundaries defined by the region limits.
        ///
        /// \param Input The placement to clamp.
        /// \return A placement clamped to the valid world extents.
        ZYPHRYON_INLINE static constexpr Placement Clamp(Placement Input)
        {
            return FromAbsolute(
                Math::Clamp(Input.GetAbsoluteX(), static_cast<Real64>(kMinTileX), static_cast<Real64>(kMaxTileX)),
                Math::Clamp(Input.GetAbsoluteY(), static_cast<Real64>(kMinTileY), static_cast<Real64>(kMaxTileY)));
        }

        /// \brief Linearly interpolates between two placements.
        ///
        /// \param Start      The starting placement.
        /// \param End        The ending placement.
        /// \param Percentage The interpolation factor in [0, 1].
        /// \return A normalized placement interpolated between Start and End.
        ZYPHRYON_INLINE static constexpr Placement Lerp(Placement Start, Placement End, Real32 Percentage)
        {
            const Real64 OffsetX = End.GetAbsoluteX() - Start.GetAbsoluteX();
            const Real64 OffsetY = End.GetAbsoluteY() - Start.GetAbsoluteY();

            return FromAbsolute(
                Start.GetAbsoluteX() + OffsetX * Percentage,
                Start.GetAbsoluteY() + OffsetY * Percentage);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        SInt16 mRegionX;
        SInt16 mRegionY;
        Real32 mOffsetX;
        Real32 mOffsetY;
    };
}