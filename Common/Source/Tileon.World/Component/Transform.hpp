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

#include <Zyphryon.Math/Transform2D.hpp>
#include <Zyphryon.Math/Geometry/Rect.hpp>
#include <Zyphryon.Scene/Builder.hpp>
#include <Zyphryon.Scene/Tag.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon
{
    /// \brief Represents a tag for entities that are considered stale or outdated.
    using EcsStale   = Scene::Tag<"Stale">;

    /// \brief Represents a tag for entities that are dynamic and subject to change.
    using EcsKinetic = Scene::Tag<"Kinetic">;

    /// \brief Alias for entities that are either kinetic or stale.
    using EcsDynamic = Scene::DSL::Or<const EcsKinetic, const EcsStale>;

    /// \brief Represents the world space transformation of an entity, including position, rotation, and scale.
    using Worldspace = Math::Matrix3x2;

    /// \brief Represents the local space transformation of an entity, including position, rotation, and scale.
    using Localspace = Math::Transform2D;

    /// \brief Represents the bounding volume of an entity, defined as a rectangle in 2D space.
    using Volume     = Math::Rect;

    /// \brief Represents the origin point of an entity, used for transformations and positioning.
    using Origin     = Math::Vector2;
}