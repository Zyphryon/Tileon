// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Copyright (C) 2025-2026 Tileon contributors (see AUTHORS.md)
//
// This work is licensed under the terms of the MIT license.
//
// For a copy, see <https://opensource.org/licenses/MIT>.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [  HEADER  ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "Registry.hpp"
#include "Descriptor.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Registry::Registry(Ref<Scene::Service> Scene)
        : mScene { Scene }
    {
        OnRegister();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Registry::OnRegister()
    {
        mScene.GetComponent<Descriptor>("Descriptor").Grant(Scene::Trait::Final);

        Add<Pose>     ("Pose",      ICON_FA_UP_DOWN_LEFT_RIGHT,     "Kinematic");
        Add<Anchor>   ("Anchor",    ICON_FA_ANCHOR,                 "Kinematic");
        Add<Extent>   ("Extent",    ICON_FA_EXPAND,                 "Volume");
        Add<Velocity> ("Velocity",  ICON_FA_GAUGE_HIGH,             "Motion");
        Add<Sprite>   ("Sprite",    ICON_FA_IMAGE,                  "Render");
        Add<Animation>("Animation", ICON_FA_FILM,                   "Render");
        Add<IntColor8>("Tint",      ICON_FA_PALETTE,                "Render");
        Add<Typeface> ("Typeface",  ICON_FA_FONT,                   "Typography");
        Add<Label>    ("Label",     ICON_FA_COMMENT,                "Typography");
        Add<Emphasis> ("Emphasis",  ICON_FA_WAND_MAGIC_SPARKLES,    "Typography");
        Add<Glowlight>("Glowlight", ICON_FA_LIGHTBULB,              "Light");
        Add<Spotlight>("Spotlight", ICON_FA_LIGHTBULB,              "Light");
        Add<Skylight> ("Skylight",  ICON_FA_SUN,                    "Light");
    }
}
