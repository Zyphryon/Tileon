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

#include "Palette.hpp"
#include "Tileon.Editor/Utility.hpp"
#include "Tileon.Editor/Panel/Viewport/Tools.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    Palette::Palette(Ref<Context> Context)
        : Panel       { Context, "Palette", true },
          mRepository { Context.GetRepository() }
    {
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::OnDraw()
    {
        Toolkit::Composer::SetNextWindowSize(300.0f, 500.0f, ImGuiCond_FirstUseEver);
        Toolkit::Composer::SetNextWindowSizeConstraints(200.0f, 300.0f, 800.0f, 1200.0f);

        if (Toolkit::Composer::Begin(GetTitle(), mVisible))
        {
            DrawEntityTab();
        }
        Toolkit::Composer::End();
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawEntityTab()
    {
        mEntities.DrawToolbar();
        Toolkit::Composer::Separator();

        const Real32 BodyHeight = -(Toolkit::Composer::GetFrameHeightWithSpacing() + 8.0f);
        Toolkit::Composer::BeginChild("##entity_body", ImVec2(0.0f, BodyHeight), ImGuiChildFlags_None);
        DrawEntityGallery();
        Toolkit::Composer::EndChild();

        DrawBottomBar("##entity_status", [&](Real32)
        {
            DrawEntityStatus();
        });
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawEntityGallery()
    {
        mEntities.SetSelection(GetContext().GetInteger("Selection.Archetype", 0));

        mEntities.Begin();
        mRepository.ForEachArchetype([&](Scene::Entity Archetype)
        {
            // Only root archetypes are placeable.
            if (Archetype.GetParent(Scene::Hierarchy::Fixed).IsValid())
            {
                return;
            }

            const UInt32 ID = static_cast<UInt32>(Archetype.GetID() - Scene::kMinRangeArchetypes);

            Graphic::Object Thumbnail = 0;
            Rect            Crop      = Rect::One();
            IntColor8       Tint      = IntColor8::White();

            if (const ConstPtr<Appearance> Visual = Archetype.TryGet<const Appearance>())
            {
                ConstRetainer<Graphic::Material> Material = Visual->GetMaterial();

                if (Material && Material->HasCompleted())
                {
                    if (ConstRetainer<Graphic::Image> Albedo = Material->GetImage(GetTextureID(TextureUsage::Albedo)))
                    {
                        Thumbnail = Albedo->GetHandle();
                        Crop      = Visual->GetSource();

                        if (const ConstPtr<IntColor8> Color = Archetype.TryGet<const IntColor8>())
                        {
                            Tint = (* Color);
                        }
                    }
                }
            }

            // An archetype that is still loading, failed, or has no sprite falls back to the gallery's placeholder.
            if (mEntities.DrawItem(ID, Archetype.GetAlias(), Thumbnail, Crop, Tint))
            {
                GetContext().SetInteger("Selection.Archetype", ID);
            }
        });
        mEntities.End();

        // Right-clicking an entity jumps to the Archetypes editor with it selected, keeping palette and editor in sync.
        if (const SInt64 Target = mEntities.GetActivated(); Target >= 0)
        {
            GetContext().SetString("Navigate.Panel", "Archetypes");
            GetContext().SetInteger("Selection.Archetype", Target);
            GetContext().SetInteger("Selection.Archetype.Target", Scene::kMinRangeArchetypes + Target);
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawEntityStatus()
    {
        const UInt32        Selection = GetContext().GetInteger("Selection.Archetype", 0);
        const Scene::Entity Archetype = mRepository.GetArchetype(Scene::kMinRangeArchetypes + Selection);

        if (Selection != 0 && Archetype.IsValid())
        {
            const Text Alias = Archetype.GetAlias();

            Toolkit::Composer::SetCursorPosX(Toolkit::Composer::GetStyle().ItemSpacing.x);
            Toolkit::Composer::Label("{0:04}  {1}", Selection, Alias.IsEmpty() ? "(Unnamed)" : Alias);
        }
        else
        {
            DrawHint("No archetype selected");
        }
    }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void Palette::DrawHint(Text Hint)
    {
        Toolkit::Composer::SetCursorPosX(
            (Toolkit::Composer::GetWindowWidth() - Toolkit::Composer::CalcTextSize(Hint).x) * 0.5f);
        Toolkit::Composer::TextDisabled(Hint);
    }
}