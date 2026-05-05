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

#include "Previewer.hpp"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::UI
{
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

     Previewer::Previewer()
         : mZoom { 1.0f }
     {
     }

    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

     void Previewer::Draw(Ref<Composer> Composer, Graphic::Object Texture, Vector2 Size, Rect Source, Color Tint)
     {
        const ImVec2 Available = Composer.GetContentRegionAvail();
        const ImVec2 Origin    = Composer.GetCursorScreenPos();

        // Reserve the entire content area so ImGui tracks hover / active state.
        Composer.InvisibleButton("##viewer_area", Available);

        const Bool   Hovered = Composer.IsItemHovered();
        const ImVec2 Mouse   = Composer.GetMousePos();

        // R key resets zoom and pan while the viewer is hovered.
        if (Hovered && Composer.IsKeyPressed(ImGuiKey_R))
        {
            Reset();
        }

        // Zoom at mouse position, keeping the point under the cursor fixed in place.
        if (Hovered)
        {
            const Real32 Wheel = Composer.GetMouseWheel();

            if (Wheel != 0.0f)
            {
                const Real32 OldZoom = mZoom;
                const Real32 Factor  = (Wheel > 0.0f) ? kZoomStep : (1.0f / kZoomStep);
                const Real32 NewZoom = Math::Clamp(mZoom * Factor, kZoomMin, kZoomMax);

                // Current display at old zoom.
                const ImVec2 OldDisplay(Size.GetX() * OldZoom, Size.GetY() * OldZoom);
                const ImVec2 ImageTL(
                    Origin.x + (Available.x - OldDisplay.x) * 0.5f + mPan.GetX(),
                    Origin.y + (Available.y - OldDisplay.y) * 0.5f + mPan.GetY());

                // Image-space coord under the mouse.
                const ImVec2 ImageCoords((Mouse.x - ImageTL.x) / OldZoom, (Mouse.y - ImageTL.y) / OldZoom);

                // Adjust pan so the point under the mouse doesn't move.
                const ImVec2 NewDisplay(Size.GetX() * NewZoom, Size.GetY() * NewZoom);
                mPan.SetX(Mouse.x - Origin.x - (Available.x - NewDisplay.x) * 0.5f - ImageCoords.x * NewZoom);
                mPan.SetY(Mouse.y - Origin.y - (Available.y - NewDisplay.y) * 0.5f - ImageCoords.y * NewZoom);
                mZoom = NewZoom;
            }
        }

        // Pan with middle mouse button, moving the image in the direction of the mouse movement.
        if (Hovered && Composer.IsMouseDragging(ImGuiMouseButton_Middle))
        {
            const ImVec2 Delta = Composer.GetMouseDelta();
            mPan += Vector2(Delta.x, Delta.y);
        }

        // Calculate the top-left and bottom-right corners of the image.
        const ImVec2 Display(Size.GetX() * mZoom, Size.GetY() * mZoom);
        const ImVec2 ImageTL(
            Origin.x + (Available.x - Display.x) * 0.5f + mPan.GetX(),
            Origin.y + (Available.y - Display.y) * 0.5f + mPan.GetY());
        const ImVec2 ImageBR(ImageTL.x + Display.x, ImageTL.y + Display.y);

        // PushClipRect restricts all draw calls to exactly that area so panning can't bleed outside.
        const Ptr<ImDrawList> DrawList = Composer.GetWindowDrawList();
        DrawList->PushClipRect(Origin, ImVec2(Origin.x + Available.x, Origin.y + Available.y), true);

        // Clear the background with a checkerboard pattern to indicate transparency.
        constexpr Real32 kTileSize = 16.0f;
        constexpr ImU32  kColorA   = IM_COL32( 80,  80,  80, 255);
        constexpr ImU32  kColorB   = IM_COL32(120, 120, 120, 255);

        const SInt32 NumCols = static_cast<SInt32>(Available.x / kTileSize) + 1;
        const SInt32 NumRows = static_cast<SInt32>(Available.y / kTileSize) + 1;

        for (SInt32 Row = 0; Row < NumRows; ++Row)
        {
            for (SInt32 Col = 0; Col < NumCols; ++Col)
            {
                const ImVec2 TL(Origin.x +  Col      * kTileSize, Origin.y +  Row      * kTileSize);
                const ImVec2 BR(Origin.x + (Col + 1) * kTileSize, Origin.y + (Row + 1) * kTileSize);
                DrawList->AddRectFilled(TL, BR, ((Row + Col) % 2 == 0) ? kColorA : kColorB);
            }
        }

        // Draw the texture if available, scaling it to fit the zoom level and centering it.
        if (Texture)
        {
            const ImVec2  UV0(Source.GetMinimumX(), Source.GetMinimumY());
            const ImVec2  UV1(Source.GetMaximumX(), Source.GetMaximumY());
            const ImColor Color(Tint.GetRed(), Tint.GetGreen(), Tint.GetBlue(), Tint.GetAlpha());

            DrawList->AddImage(Texture, ImageTL, ImageBR, UV0, UV1, Color);
        }

        // Draw a border around the image area.
        if (Size.GetX() > 0.0f && Size.GetY() > 0.0f)
        {
            DrawList->AddRect(ImageTL, ImageBR, IM_COL32(255, 255, 255, 120), 0.0f, 0, 1.0f);
        }

        // Draw the status bar in the bottom-right corner (zoom + pixel coords when hovered).
        {
            const ConstStr8 ZoomLabel = Base::Format("{:.0f}%", mZoom * 100.0f);

            ConstStr8 CoordLabel;
            if (Size.GetX() > 0.0f && Size.GetY() > 0.0f && Hovered)
            {
                const Real32 PX = Math::Clamp((Mouse.x - ImageTL.x) / mZoom, 0.0f, Size.GetX() - 1.0f);
                const Real32 PY = Math::Clamp((Mouse.y - ImageTL.y) / mZoom, 0.0f, Size.GetY() - 1.0f);
                CoordLabel = Base::Format("X: {:.0f}  Y: {:.0f}    {}    R to reset", PX, PY, ZoomLabel);
            }
            else
            {
                CoordLabel = Base::Format("{}    R to reset", ZoomLabel);
            }

            const ImVec2 LabelSize = Composer.CalcTextSize(CoordLabel);
            const ImVec2 LabelPos(Origin.x + Available.x - LabelSize.x - 6.0f, Origin.y + Available.y - LabelSize.y - 6.0f);
            DrawList->AddText(LabelPos, IM_COL32(255, 255, 255, 180), CoordLabel.data());
        }

        DrawList->PopClipRect();
     }
}