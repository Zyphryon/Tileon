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

#include "Icon.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <Zyphryon.Graphic/Common.hpp>
#include <Zyphryon.Math/Color.hpp>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::UI
{
    /// \brief Provides utility functions for rendering user interface elements in the editor.
    class Composer final
    {
    public:

        ZYPHRYON_INLINE Bool Begin(ConstStr8 Title, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            return ImGui::Begin(Title.data(), nullptr, Flags);
        }

        ZYPHRYON_INLINE Bool Begin(ConstStr8 Title, Ref<Bool> Open, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            return ImGui::Begin(Title.data(), &Open, Flags);
        }

        ZYPHRYON_INLINE void End()
        {
            ImGui::End();
        }

        ZYPHRYON_INLINE void BeginChild(ConstStr8 ID, ImVec2 Size = ImVec2(0, 0), ImGuiChildFlags ChildFlags = ImGuiChildFlags_None, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            ImGui::BeginChild(ID.data(), Size, ChildFlags, Flags);
        }

        ZYPHRYON_INLINE void EndChild()
        {
            ImGui::EndChild();
        }

        ZYPHRYON_INLINE Bool BeginMainMenuBar()
        {
            return ImGui::BeginMainMenuBar();
        }

        ZYPHRYON_INLINE void EndMainMenuBar()
        {
            ImGui::EndMainMenuBar();
        }

        ZYPHRYON_INLINE Bool BeginMenu(ConstStr8 Label, Bool Enabled = true)
        {
            return ImGui::BeginMenu(Label.data(), Enabled);
        }

        ZYPHRYON_INLINE void EndMenu()
        {
            ImGui::EndMenu();
        }

        ZYPHRYON_INLINE void SetNextWindowSize(Real32 Width, Real32 Height, ImGuiCond Condition = ImGuiCond_None)
        {
            ImGui::SetNextWindowSize(ImVec2(Width, Height), Condition);
        }

        ZYPHRYON_INLINE void SetNextWindowSizeConstraints(Real32 MinWidth, Real32 MinHeight, Real32 MaxWidth = FLT_MAX, Real32 MaxHeight = FLT_MAX)
        {
            ImGui::SetNextWindowSizeConstraints(ImVec2(MinWidth, MinHeight), ImVec2(MaxWidth, MaxHeight));
        }

        ZYPHRYON_INLINE void SameLine(Real32 OffsetX = 0.0f, Real32 Spacing = -1.0f)
        {
            ImGui::SameLine(OffsetX, Spacing);
        }

        ZYPHRYON_INLINE void Spacing()
        {
            ImGui::Spacing();
        }

        ZYPHRYON_INLINE void Separator()
        {
            ImGui::Separator();
        }

        ZYPHRYON_INLINE void SeparatorEx(ImGuiSeparatorFlags Flags = ImGuiSeparatorFlags_None)
        {
            ImGui::SeparatorEx(Flags);
        }

        ZYPHRYON_INLINE ImVec2 GetContentRegionAvail() const
        {
            return ImGui::GetContentRegionAvail();
        }

        ZYPHRYON_INLINE ImVec2 GetCursorScreenPos() const
        {
            return ImGui::GetCursorScreenPos();
        }

        ZYPHRYON_INLINE Real32 GetFrameHeightWithSpacing() const
        {
            return ImGui::GetFrameHeightWithSpacing();
        }

        ZYPHRYON_INLINE Real32 GetTextLineHeight() const
        {
            return ImGui::GetTextLineHeight();
        }

        ZYPHRYON_INLINE void SetCursorScreenPos(ImVec2 Pos)
        {
            ImGui::SetCursorScreenPos(Pos);
        }

        ZYPHRYON_INLINE ImVec2 CalcTextSize(ConstStr8 Text) const
        {
            return ImGui::CalcTextSize(Text.data());
        }

        ZYPHRYON_INLINE void PushItemWidth(Real32 Width)
        {
            ImGui::PushItemWidth(Width);
        }

        ZYPHRYON_INLINE void PopItemWidth()
        {
            ImGui::PopItemWidth();
        }

        ZYPHRYON_INLINE void SetNextItemWidth(Real32 Width)
        {
            ImGui::SetNextItemWidth(Width);
        }

        ZYPHRYON_INLINE void SetCursorPosX(Real32 X)
        {
            ImGui::SetCursorPosX(X);
        }

        ZYPHRYON_INLINE void SetCursorPosY(Real32 Y)
        {
            ImGui::SetCursorPosY(Y);
        }

        ZYPHRYON_INLINE void SetScrollHereY(Real32 CenterRatio = 0.5f)
        {
            ImGui::SetScrollHereY(CenterRatio);
        }

        ZYPHRYON_INLINE ImVec2 GetItemRectMin() const
        {
            return ImGui::GetItemRectMin();
        }

        ZYPHRYON_INLINE ImVec2 GetItemRectSize() const
        {
            return ImGui::GetItemRectSize();
        }

        ZYPHRYON_INLINE ImVec2 GetMousePos() const
        {
            return ImGui::GetMousePos();
        }

        ZYPHRYON_INLINE ImVec2 GetMouseDelta() const
        {
            return ImGui::GetIO().MouseDelta;
        }

        ZYPHRYON_INLINE Real32 GetMouseWheel() const
        {
            return ImGui::GetIO().MouseWheel;
        }

        ZYPHRYON_INLINE Bool IsMouseClicked(ImGuiMouseButton Button = ImGuiMouseButton_Left, Bool Repeat = false) const
        {
            return ImGui::IsMouseClicked(Button, Repeat);
        }

        ZYPHRYON_INLINE Bool IsMouseDragging(ImGuiMouseButton Button = ImGuiMouseButton_Left, Real32 LockThreshold = -1.0f) const
        {
            return ImGui::IsMouseDragging(Button, LockThreshold);
        }

        ZYPHRYON_INLINE Bool IsMouseHoveringRect(ImVec2 Min, ImVec2 Max, Bool Clip = true) const
        {
            return ImGui::IsMouseHoveringRect(Min, Max, Clip);
        }

        ZYPHRYON_INLINE ConstRef<ImGuiStyle> GetStyle() const
        {
            return ImGui::GetStyle();
        }

        ZYPHRYON_INLINE ImVec4 GetStyleColorVec4(ImGuiCol Index) const
        {
            return ImGui::GetStyleColorVec4(Index);
        }

        ZYPHRYON_INLINE Ptr<ImDrawList> GetWindowDrawList() const
        {
            return ImGui::GetWindowDrawList();
        }

        ZYPHRYON_INLINE void BeginDisabled(Bool Disabled = true)
        {
            ImGui::BeginDisabled(Disabled);
        }

        ZYPHRYON_INLINE void EndDisabled()
        {
            ImGui::EndDisabled();
        }

        ZYPHRYON_INLINE void Header(ConstStr8 Label)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.70f, 0.85f, 1.0f));
            ImGui::SeparatorText(Label.data());
            ImGui::PopStyleColor();
        }

        ZYPHRYON_INLINE void Section(ConstStr8 Label)
        {
            Header(Label);
            Spacing();
        }

        ZYPHRYON_INLINE void Field(ConstStr8 Label)
        {
            PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::TextUnformatted(Label.data(), Label.data() + Label.size());
            PopStyleColor();
        }

        ZYPHRYON_INLINE void Indent(Real32 Width = 0.0f)
        {
            ImGui::Indent(Width);
        }

        ZYPHRYON_INLINE void Unindent(Real32 Width = 0.0f)
        {
            ImGui::Unindent(Width);
        }

        template<typename... Arguments>
        ZYPHRYON_INLINE void Label(ConstStr8 Format, AnyRef<Arguments>... Parameters)
        {
            ImGui::AlignTextToFramePadding();

            const ConstStr8 Text = Base::Format(Format, Parameters...);
            ImGui::TextUnformatted(Text.data(), Text.data() + Text.size());
        }

        template<typename... Arguments>
        ZYPHRYON_INLINE void TextDisabled(ConstStr8 Format, AnyRef<Arguments>... Parameters)
        {
            const ConstStr8 Text = Base::Format(Format, Parameters...);

            PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::TextUnformatted(Text.data(), Text.data() + Text.size());
            PopStyleColor();
        }

        template<typename... Arguments>
        ZYPHRYON_INLINE void TextColored(ImVec4 Color, ConstStr8 Format, AnyRef<Arguments>... Parameters)
        {
            const ConstStr8 Text = Base::Format(Format, Parameters...);

            PushStyleColor(ImGuiCol_Text, Color);
            ImGui::TextUnformatted(Text.data(), Text.data() + Text.size());
            PopStyleColor();
        }

        ZYPHRYON_INLINE Real32 GetWindowWidth() const
        {
            return ImGui::GetWindowWidth();
        }

        ZYPHRYON_INLINE Real32 GetCursorPosX() const
        {
            return ImGui::GetCursorPosX();
        }

        ZYPHRYON_INLINE void Image(Graphic::Object ID, ImVec2 Size, ImVec4 UV = ImVec4(0, 0, 1, 1))
        {
            const ImVec2 UV0(UV.x, UV.y);
            const ImVec2 UV1(UV.z, UV.w);
            ImGui::Image(ID, ImVec2(Size.x, Size.y), UV0, UV1);
        }

        ZYPHRYON_INLINE void PushStyleColor(ImGuiCol Index, ImVec4 Color)
        {

            ImGui::PushStyleColor(Index, Color);
        }

        ZYPHRYON_INLINE void PopStyleColor(SInt32 Count = 1)
        {
            ImGui::PopStyleColor(Count);
        }

        ZYPHRYON_INLINE Bool MenuItem(ConstStr8 Label, ConstStr8 Shortcut = {}, Bool Enabled = true)
        {
            return ImGui::MenuItem(Label.data(), Shortcut.empty() ? nullptr : Shortcut.data(), false, Enabled);
        }

        ZYPHRYON_INLINE Bool Button(ConstStr8 Label, Real32 Width = 0.0f, Real32 Height = 0.0f)
        {
            return ImGui::Button(Label.data(), ImVec2(Width, Height));
        }

        ZYPHRYON_INLINE Bool DisabledButton(ConstStr8 Label, Bool Disable, Real32 Width = 0.0f, Real32 Height = 0.0f)
        {
            ImGui::BeginDisabled(Disable);

            const Bool Result = Button(Label, Width, Height) && !Disable;

            ImGui::EndDisabled();

            return Result;
        }

        ZYPHRYON_INLINE Bool SmallButton(ConstStr8 Label)
        {
            return ImGui::SmallButton(Label.data());
        }

        ZYPHRYON_INLINE Bool Checkbox(ConstStr8 Label, Ref<Bool> State)
        {
            return ImGui::Checkbox(Label.data(), &State);
        }

        ZYPHRYON_INLINE Bool RadioButton(ConstStr8 Label, Bool Active)
        {
            return ImGui::RadioButton(Label.data(), Active);
        }

        ZYPHRYON_INLINE Bool Selectable(ConstStr8 Label, Bool Selected = false, ImGuiSelectableFlags Flags = ImGuiSelectableFlags_None)
        {
            return ImGui::Selectable(Label.data(), Selected, Flags);
        }

        ZYPHRYON_INLINE Bool IsItemClicked(ImGuiMouseButton Button = ImGuiMouseButton_Left)
        {
            return ImGui::IsItemClicked(Button);
        }

        ZYPHRYON_INLINE Bool IsItemHovered(ImGuiHoveredFlags Flags = ImGuiHoveredFlags_None) const
        {
            return ImGui::IsItemHovered(Flags);
        }

        ZYPHRYON_INLINE Bool IsKeyPressed(ImGuiKey Key, Bool Repeat = false) const
        {
            return ImGui::IsKeyPressed(Key, Repeat);
        }

        ZYPHRYON_INLINE Bool InvisibleButton(ConstStr8 ID, ImVec2 Size, ImGuiButtonFlags Flags = ImGuiButtonFlags_None)
        {
            return ImGui::InvisibleButton(ID.data(), Size, Flags);
        }

        ZYPHRYON_INLINE Bool BeginPopupContextItem(ConstStr8 ID = {})
        {
            return ImGui::BeginPopupContextItem(ID.empty() ? nullptr : ID.data());
        }

        ZYPHRYON_INLINE void EndPopup()
        {
            ImGui::EndPopup();
        }

        ZYPHRYON_INLINE Bool BeginTabBar(ConstStr8 ID, ImGuiTabBarFlags Flags = ImGuiTabBarFlags_None)
        {
            return ImGui::BeginTabBar(ID.data(), Flags);
        }

        ZYPHRYON_INLINE void EndTabBar()
        {
            ImGui::EndTabBar();
        }

        ZYPHRYON_INLINE Bool BeginTabItem(ConstStr8 Label, ImGuiTabItemFlags Flags = ImGuiTabItemFlags_None)
        {
            return ImGui::BeginTabItem(Label.data(), nullptr, Flags);
        }

        ZYPHRYON_INLINE void EndTabItem()
        {
            ImGui::EndTabItem();
        }

        ZYPHRYON_INLINE Bool BeginCombo(ConstStr8 ID, ConstStr8 Preview, ImGuiComboFlags Flags = ImGuiComboFlags_None)
        {
            return ImGui::BeginCombo(ID.data(), Preview.data(), Flags);
        }

        ZYPHRYON_INLINE void EndCombo()
        {
            ImGui::EndCombo();
        }

        ZYPHRYON_INLINE Bool BeginTable(ConstStr8 ID, SInt32 Columns, ImGuiTableFlags Flags = ImGuiTableFlags_None, ImVec2 Size = ImVec2(0, 0))
        {
            return ImGui::BeginTable(ID.data(), Columns, Flags, Size);
        }

        ZYPHRYON_INLINE void EndTable()
        {
            ImGui::EndTable();
        }

        ZYPHRYON_INLINE void TableSetupColumn(ConstStr8 Label, ImGuiTableColumnFlags Flags = ImGuiTableColumnFlags_None, Real32 InitWidth = 0.0f)
        {
            ImGui::TableSetupColumn(Label.data(), Flags, InitWidth);
        }

        ZYPHRYON_INLINE void TableHeadersRow()
        {
            ImGui::TableHeadersRow();
        }

        ZYPHRYON_INLINE void TableNextRow(ImGuiTableRowFlags Flags = ImGuiTableRowFlags_None, Real32 MinHeight = 0.0f)
        {
            ImGui::TableNextRow(Flags, MinHeight);
        }

        ZYPHRYON_INLINE Bool TableSetColumnIndex(SInt32 Column)
        {
            return ImGui::TableSetColumnIndex(Column);
        }

        ZYPHRYON_INLINE Bool InputFloat(ConstStr8 ID, Ref<Real32> Value, Real32 Step = 0.0f, Real32 StepFast = 0.0f, ConstStr8 Format = "%.2f", ImGuiInputTextFlags Flags = ImGuiInputTextFlags_None)
        {
            return ImGui::InputFloat(ID.data(), &Value, Step, StepFast, Format.data(), Flags);
        }

        template<typename Type>
        ZYPHRYON_INLINE Bool InputInt(ConstStr8 ID, Ref<Type> Value, Type Step = Type(1), Type StepFast = Type(1), ImGuiInputTextFlags Flags = ImGuiInputTextFlags_None)
        {
            int Temp = static_cast<int>(Value);

            if (ImGui::InputInt(ID.data(), &Temp, Step, StepFast, Flags))
            {
                Value = static_cast<Type>(Temp);
                return true;
            }
            return false;
        }

        template<typename Type>
        ZYPHRYON_INLINE Bool InputIntPair(
            ConstStr8           ID,
            Ref<Type>           X,
            Ref<Type>           Y,
            ConstStr8           Separator = "x",
            Type                Step      = Type(1),
            Type                StepFast  = Type(1),
            ImGuiInputTextFlags Flags     = ImGuiInputTextFlags_None)
        {
            const Real32 Size    = ImGui::CalcTextSize(Separator.data()).x;
            const Real32 Spacing = ImGui::GetStyle().ItemSpacing.x;
            const Real32 Width   = (ImGui::GetContentRegionAvail().x - Size - Spacing * 2.0f) * 0.5f;

            Bool Dirty = false;

            ImGui::SetNextItemWidth(Width);
            Dirty |= InputInt(Base::Format("{}_x", ID), X, Step, StepFast, Flags);

            ImGui::SameLine();
            ImGui::TextDisabled("%s", Separator.data());
            ImGui::SameLine();

            ImGui::SetNextItemWidth(Width);
            Dirty |= InputInt(Base::Format("{}_y", ID), Y, Step, StepFast, Flags);

            return Dirty;
        }

        template<typename Callback>
        ZYPHRYON_INLINE void InputText(
            ConstStr8           ID,
            ConstStr8           Value,
            AnyRef<Callback>    Action,
            ImGuiInputTextFlags Flags = ImGuiInputTextFlags_None)
        {
            static char kBuffer[256];
            strncpy_s(kBuffer, sizeof(kBuffer), Value.data(), sizeof(kBuffer) - 1);

            if (ImGui::InputText(ID.data(), kBuffer, sizeof(kBuffer), Flags))
            {
                Action(ConstStr8(kBuffer));
            }
        }

        template<typename InputCallback, typename ButtonCallback>
        ZYPHRYON_INLINE void InputTextWithButton(
            ConstStr8              ID,
            ConstStr8              TextValue,
            AnyRef<InputCallback>  TextAction,
            ConstStr8              ButtonLabel,
            AnyRef<ButtonCallback> ButtonAction,
            ImGuiInputTextFlags    Flags = ImGuiInputTextFlags_None)
        {
            const Real32 Width = ImGui::CalcTextSize(ButtonLabel.data()).x + ImGui::GetStyle().FramePadding.x * 2.0f;
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - Width - ImGui::GetStyle().ItemSpacing.x);
            InputText(ID, TextValue, TextAction, Flags);

            ImGui::SameLine();
            if (ImGui::SmallButton(ButtonLabel.data()))
            {
                ButtonAction();
            }
        }

        template<typename Tint>
        ZYPHRYON_INLINE Bool InputTint(ConstStr8 ID, Ref<Tint> Value, ImGuiColorEditFlags Flags = ImGuiColorEditFlags_None)
        {
            constexpr Bool IsFloatColor = IsEqual<Tint, Color>;

            Real32 RGBA[] = {
                IsFloatColor ? Value.GetRed()   : Value.GetRed()   / 255.0f,
                IsFloatColor ? Value.GetGreen() : Value.GetGreen() / 255.0f,
                IsFloatColor ? Value.GetBlue()  : Value.GetBlue()  / 255.0f,
                IsFloatColor ? Value.GetAlpha() : Value.GetAlpha() / 255.0f
            };

            if (ImGui::ColorEdit4(ID.data(), RGBA, Flags))
            {
                if constexpr (IsFloatColor)
                {
                    Value = Color(RGBA[0], RGBA[1], RGBA[2], RGBA[3]);
                }
                else
                {
                    Value = IntColor8(RGBA[0] * 255, RGBA[1] * 255, RGBA[2] * 255, RGBA[3] * 255);
                }
                return true;
            }
            return false;
        }

        template<typename Tint>
        ZYPHRYON_INLINE Bool InputTintSmall(ConstStr8 ID, Ref<Tint> Value)
        {
            constexpr ImGuiColorEditFlags Flags =
                ImGuiColorEditFlags_AlphaBar |
                ImGuiColorEditFlags_AlphaPreviewHalf |
                ImGuiColorEditFlags_Uint8 |
                ImGuiColorEditFlags_DisplayRGB;
            return InputTint(ID, Value, Flags);
        }
    };
}

