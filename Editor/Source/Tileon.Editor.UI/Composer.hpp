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
#include <Zyphryon.Graphic/Types.hpp>
#include <imgui.h>
#include <imgui_internal.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::UI
{
    /// \brief Builds an initial dock layout by splitting a root node and assigning windows to the pieces.
    class Dock final
    {
    public:

        /// \brief Constructs a builder around the dockspace's root node.
        ///
        /// \param Root The identifier of the dockspace's root node.
        ZY_INLINE explicit Dock(ImGuiID Root)
            : mRoot { Root }
        {
        }

        /// \brief Gets the root node, which also represents the area remaining after edges are split off.
        ///
        /// \return The root node identifier.
        ZY_INLINE ImGuiID GetRoot() const
        {
            return mRoot;
        }

        /// \brief Carves a piece off a node toward an edge and returns it, leaving the remainder in \p Node.
        ///
        /// \param Node      The node to split; on return it holds the remaining area.
        /// \param Direction The edge to carve the new piece from.
        /// \param Ratio     The fraction (0..1) of \p Node the carved piece occupies.
        /// \return The node representing the carved-off piece.
        ZY_INLINE ImGuiID Split(Ref<ImGuiID> Node, ImGuiDir Direction, Real32 Ratio) const
        {
            return ImGui::DockBuilderSplitNode(Node, Direction, Ratio, nullptr, AddressOf(Node));
        }

        /// \brief Assigns a window to a node by its title.
        ///
        /// \param Window The title of the window to dock, as passed to \ref Composer::Begin.
        /// \param Node   The node the window should occupy.
        ZY_INLINE void Attach(Text Window, ImGuiID Node) const
        {
            ImGui::DockBuilderDockWindow(Window.GetData(), Node);
        }

    private:

        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
        // -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        ImGuiID mRoot;
    };

    /// \brief Provides utility functions for rendering user interface elements in the editor.
    class Composer final
    {
    public:

        static constexpr ImVec4 kAxisTintX  = ImVec4(0.72f, 0.28f, 0.30f, 1.0f);
        static constexpr ImVec4 kAxisTintY  = ImVec4(0.38f, 0.64f, 0.32f, 1.0f);

    public:

        ZY_INLINE Bool Begin(Text Title, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            return ImGui::Begin(Title.GetData(), nullptr, Flags);
        }

        ZY_INLINE Bool Begin(Text Title, Ref<Bool> Open, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            return ImGui::Begin(Title.GetData(), AddressOf(Open), Flags);
        }

        ZY_INLINE void End()
        {
            ImGui::End();
        }

        template<typename Function>
        ZY_INLINE void DockSpace(Text ID, AnyRef<Function> Builder)
        {
            const ImGuiID Node = ImGui::GetID(ID.GetData());

            if (ImGui::DockBuilderGetNode(Node) == nullptr)
            {
                ImGui::DockBuilderAddNode(Node, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(Node, ImGui::GetMainViewport()->WorkSize);

                Dock Layout(Node);
                Builder(Layout);

                ImGui::DockBuilderFinish(Node);
            }

            ImGui::DockSpaceOverViewport(Node, ImGui::GetMainViewport());
        }

        ZY_INLINE void ResetDockSpace(Text ID)
        {
            ImGui::DockBuilderRemoveNode(ImGui::GetID(ID.GetData()));
        }

        ZY_INLINE void BeginChild(Text ID, ImVec2 Size = ImVec2(0, 0), ImGuiChildFlags ChildFlags = ImGuiChildFlags_None, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            ImGui::BeginChild(ID.GetData(), Size, ChildFlags, Flags);
        }

        ZY_INLINE void EndChild()
        {
            ImGui::EndChild();
        }

        ZY_INLINE Bool TreeNode(Text Label, ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_None)
        {
            return ImGui::TreeNodeEx(Label.GetData(), Flags);
        }

        ZY_INLINE void TreePop()
        {
            ImGui::TreePop();
        }

        ZY_INLINE void SetNextItemOpen(Bool Open, ImGuiCond Condition = ImGuiCond_None)
        {
            ImGui::SetNextItemOpen(Open, Condition);
        }

        ZY_INLINE void PushID(Text ID)
        {
            ImGui::PushID(ID.GetData(), ID.GetData() + ID.GetSize());
        }

        ZY_INLINE void PushID(SInt32 ID)
        {
            ImGui::PushID(ID);
        }

        ZY_INLINE void PushID(ConstPtr<void> ID)
        {
            ImGui::PushID(ID);
        }

        ZY_INLINE void PopID()
        {
            ImGui::PopID();
        }

        ZY_INLINE Bool GetState(Text ID, Bool Fallback = false) const
        {
            return ImGui::GetStateStorage()->GetBool(ImGui::GetID(ID.GetData(), ID.GetData() + ID.GetSize()), Fallback);
        }

        ZY_INLINE void SetState(Text ID, Bool State)
        {
            ImGui::GetStateStorage()->SetBool(ImGui::GetID(ID.GetData(), ID.GetData() + ID.GetSize()), State);
        }

        ZY_INLINE Bool BeginMainMenuBar()
        {
            return ImGui::BeginMainMenuBar();
        }

        ZY_INLINE void EndMainMenuBar()
        {
            ImGui::EndMainMenuBar();
        }

        ZY_INLINE Bool BeginMenu(Text Label, Bool Enabled = true)
        {
            return ImGui::BeginMenu(Label.GetData(), Enabled);
        }

        ZY_INLINE void EndMenu()
        {
            ImGui::EndMenu();
        }

        ZY_INLINE ImVec2 GetViewportCenter() const
        {
            return ImGui::GetMainViewport()->GetCenter();
        }

        ZY_INLINE void SetNextWindowPos(ImVec2 Position, ImGuiCond Condition = ImGuiCond_None, ImVec2 Pivot = ImVec2(0.0f, 0.0f))
        {
            ImGui::SetNextWindowPos(Position, Condition, Pivot);
        }

        ZY_INLINE void SetNextWindowSize(Real32 Width, Real32 Height, ImGuiCond Condition = ImGuiCond_None)
        {
            ImGui::SetNextWindowSize(ImVec2(Width, Height), Condition);
        }

        ZY_INLINE void SetNextWindowSizeConstraints(Real32 MinWidth, Real32 MinHeight, Real32 MaxWidth = FLT_MAX, Real32 MaxHeight = FLT_MAX)
        {
            ImGui::SetNextWindowSizeConstraints(ImVec2(MinWidth, MinHeight), ImVec2(MaxWidth, MaxHeight));
        }

        ZY_INLINE ImVec2 GetDisplaySize() const
        {
            return ImGui::GetIO().DisplaySize;
        }

        ZY_INLINE Real32 GetWindowBottom(UInt32 Rows = 1) const
        {
            return ImGui::GetWindowHeight()
                - ImGui::GetFrameHeightWithSpacing() * static_cast<Real32>(Rows)
                - ImGui::GetStyle().WindowPadding.y;
        }

        ZY_INLINE void SliderFloat(Text ID, Ref<Real32> Value, Real32 Min, Real32 Max, Text Format = "%.2f")
        {
            ImGui::SliderFloat(ID.GetData(), AddressOf(Value), Min, Max, Format.GetData());
        }

        ZY_INLINE Bool DragFloat(
            Text             ID,
            Ref<Real32>      Value,
            Real32           Speed  = 0.1f,
            Real32           Min    = 0.0f,
            Real32           Max    = 0.0f,
            Text             Format = "%.2f",
            ImGuiSliderFlags Flags  = ImGuiSliderFlags_None)
        {
            return ImGui::DragFloat(ID.GetData(), AddressOf(Value), Speed, Min, Max, Format.GetData(), Flags);
        }

        ZY_INLINE Bool SliderAngle(Text ID, Ref<Real32> Radians, Real32 Min = 0.0f, Real32 Max = 360.0f, Text Format = "%.1f°")
        {
            return ImGui::SliderAngle(ID.GetData(), AddressOf(Radians), Min, Max, Format.GetData());
        }

        ZY_INLINE void BeginTooltip()
        {
            ImGui::BeginTooltip();
        }

        ZY_INLINE void EndTooltip()
        {
            ImGui::EndTooltip();
        }

        ZY_INLINE void SetTooltip(Text Text)
        {
            if (ImGui::BeginTooltipEx(ImGuiTooltipFlags_OverridePrevious, ImGuiWindowFlags_None))
            {
                ImGui::TextUnformatted(Text.GetData(), Text.GetData() + Text.GetSize());
                ImGui::EndTooltip();
            }
        }

        ZY_INLINE void Tooltip(Text Text)
        {
            if (!Text.IsEmpty() && IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            {
                SetTooltip(Text);
            }
        }

        ZY_INLINE void SameLine(Real32 OffsetX = 0.0f, Real32 Spacing = -1.0f)
        {
            ImGui::SameLine(OffsetX, Spacing);
        }

        ZY_INLINE void Spacing()
        {
            ImGui::Spacing();
        }

        ZY_INLINE void Separator()
        {
            ImGui::Separator();
        }

        ZY_INLINE void SeparatorEx(ImGuiSeparatorFlags Flags = ImGuiSeparatorFlags_None)
        {
            ImGui::SeparatorEx(Flags);
        }

        ZY_INLINE ImVec2 GetContentRegionAvail() const
        {
            return ImGui::GetContentRegionAvail();
        }

        ZY_INLINE ImVec2 GetCursorScreenPos() const
        {
            return ImGui::GetCursorScreenPos();
        }

        ZY_INLINE Real32 GetFrameHeightWithSpacing() const
        {
            return ImGui::GetFrameHeightWithSpacing();
        }

        ZY_INLINE Real32 GetTextLineHeight() const
        {
            return ImGui::GetTextLineHeight();
        }

        ZY_INLINE void SetCursorScreenPos(ImVec2 Pos)
        {
            ImGui::SetCursorScreenPos(Pos);
        }

        ZY_INLINE ImVec2 CalcTextSize(Text Text) const
        {
            return ImGui::CalcTextSize(Text.GetData(), Text.GetData() + Text.GetSize());
        }

        ZY_INLINE void PushItemWidth(Real32 Width)
        {
            ImGui::PushItemWidth(Width);
        }

        ZY_INLINE void PopItemWidth()
        {
            ImGui::PopItemWidth();
        }

        ZY_INLINE void SetNextItemWidth(Real32 Width)
        {
            ImGui::SetNextItemWidth(Width);
        }

        ZY_INLINE void SetCursorPosX(Real32 X)
        {
            ImGui::SetCursorPosX(X);
        }

        ZY_INLINE void SetCursorPosY(Real32 Y)
        {
            ImGui::SetCursorPosY(Y);
        }

        ZY_INLINE void SetScrollHereY(Real32 CenterRatio = 0.5f)
        {
            ImGui::SetScrollHereY(CenterRatio);
        }

        ZY_INLINE ImVec2 GetItemRectMin() const
        {
            return ImGui::GetItemRectMin();
        }

        ZY_INLINE ImVec2 GetItemRectSize() const
        {
            return ImGui::GetItemRectSize();
        }

        ZY_INLINE ImVec2 GetMousePos() const
        {
            return ImGui::GetMousePos();
        }

        ZY_INLINE ImVec2 GetMouseDelta() const
        {
            return ImGui::GetIO().MouseDelta;
        }

        ZY_INLINE Real32 GetMouseWheel() const
        {
            return ImGui::GetIO().MouseWheel;
        }

        ZY_INLINE Bool IsMouseClicked(ImGuiMouseButton Button = ImGuiMouseButton_Left, Bool Repeat = false) const
        {
            return ImGui::IsMouseClicked(Button, Repeat);
        }

        ZY_INLINE Bool IsMouseDragging(ImGuiMouseButton Button = ImGuiMouseButton_Left, Real32 LockThreshold = -1.0f) const
        {
            return ImGui::IsMouseDragging(Button, LockThreshold);
        }

        ZY_INLINE Bool IsMouseHoveringRect(ImVec2 Min, ImVec2 Max, Bool Clip = true) const
        {
            return ImGui::IsMouseHoveringRect(Min, Max, Clip);
        }

        ZY_INLINE ConstRef<ImGuiStyle> GetStyle() const
        {
            return ImGui::GetStyle();
        }

        ZY_INLINE ImU32 GetColorU32(ImGuiCol Index) const
        {
            return ImGui::GetColorU32(Index);
        }

        ZY_INLINE ImU32 GetColorU32(ImGuiCol Index, Real32 Alpha) const
        {
            return ImGui::GetColorU32(Index, Alpha);
        }

        ZY_INLINE void PushStyleVar(ImGuiStyleVar Index, Real32 Value)
        {
            ImGui::PushStyleVar(Index, Value);
        }

        ZY_INLINE void PushStyleVar(ImGuiStyleVar Index, ImVec2 Value)
        {
            ImGui::PushStyleVar(Index, Value);
        }

        ZY_INLINE void PopStyleVar(SInt32 Count = 1)
        {
            ImGui::PopStyleVar(Count);
        }

        ZY_INLINE ImVec4 GetStyleColorVec4(ImGuiCol Index) const
        {
            return ImGui::GetStyleColorVec4(Index);
        }

        ZY_INLINE Ptr<ImDrawList> GetWindowDrawList() const
        {
            return ImGui::GetWindowDrawList();
        }

        ZY_INLINE void BeginDisabled(Bool Disabled = true)
        {
            ImGui::BeginDisabled(Disabled);
        }

        ZY_INLINE void EndDisabled()
        {
            ImGui::EndDisabled();
        }

        ZY_INLINE Bool IsDisabled() const
        {
            return (ImGui::GetCurrentContext()->CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
        }

        ZY_INLINE void Header(Text Label)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.70f, 0.85f, 1.0f));
            ImGui::SeparatorTextEx(0, Label.GetData(), Label.GetData() + Label.GetSize(), 0.0f);
            ImGui::PopStyleColor();
        }

        ZY_INLINE void Section(Text Label)
        {
            Header(Label);
            Spacing();
        }

        ZY_INLINE void Field(Text Label)
        {
            PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::TextUnformatted(Label.GetData(), Label.GetData() + Label.GetSize());
            PopStyleColor();
        }

        ZY_INLINE void FieldInline(Text Label, Real32 LabelWidth = 110.0f)
        {
            const Real32 Origin = ImGui::GetCursorPosX();

            ImGui::AlignTextToFramePadding();
            PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::TextUnformatted(Label.GetData(), Label.GetData() + Label.GetSize());
            PopStyleColor();

            ImGui::SameLine();

            if (const Real32 Column = Origin + LabelWidth; ImGui::GetCursorPosX() < Column)
            {
                ImGui::SetCursorPosX(Column);
            }
            ImGui::SetNextItemWidth(-1.0f);
        }

        ZY_INLINE void Indent(Real32 Width = 0.0f)
        {
            ImGui::Indent(Width);
        }

        ZY_INLINE void Unindent(Real32 Width = 0.0f)
        {
            ImGui::Unindent(Width);
        }

        ZY_INLINE void Label(Text Formatted)
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(Formatted.GetData(), Formatted.GetData() + Formatted.GetSize());
        }

        template<typename... Arguments>
        ZY_INLINE void Label(Text Format, AnyRef<Arguments>... Parameters)
        {
            ImGui::AlignTextToFramePadding();

            String<1024> Text;
            Text.Format(Format::Pattern(Format), Parameters...);
            ImGui::TextUnformatted(Text.GetData(), Text.GetData() + Text.GetSize());
        }

        ZY_INLINE void TextDisabled(Text Formatted)
        {
            PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::TextUnformatted(Formatted.GetData(), Formatted.GetData() + Formatted.GetSize());
            PopStyleColor();
        }

        template<typename... Arguments>
        ZY_INLINE void TextDisabled(Text Format, AnyRef<Arguments>... Parameters)
        {
            String<1024> Text;
            Text.Format(Format::Pattern(Format), Parameters...);

            PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::TextUnformatted(Text.GetData(), Text.GetData() + Text.GetSize());
            PopStyleColor();
        }

        ZY_INLINE void TextColored(ConstRef<ImVec4> Color, Text Formatted)
        {
            PushStyleColor(ImGuiCol_Text, Color);
            ImGui::TextUnformatted(Formatted.GetData(), Formatted.GetData() + Formatted.GetSize());
            PopStyleColor();
        }

        template<typename... Arguments>
        ZY_INLINE void TextColored(ConstRef<ImVec4> Color, Text Format, AnyRef<Arguments>... Parameters)
        {
            String<1024> Text;
            Text.Format(Format::Pattern(Format), Parameters...);

            PushStyleColor(ImGuiCol_Text, Color);
            ImGui::TextUnformatted(Text.GetData(), Text.GetData() + Text.GetSize());
            PopStyleColor();
        }

        ZY_INLINE Real32 GetWindowWidth() const
        {
            return ImGui::GetWindowWidth();
        }

        ZY_INLINE Real32 GetWindowHeight() const
        {
            return ImGui::GetWindowHeight();
        }

        ZY_INLINE Real32 GetCursorPosX() const
        {
            return ImGui::GetCursorPosX();
        }

        ZY_INLINE void Image(Graphic::Object ID, ImVec2 Size, ImVec4 UV = ImVec4(0, 0, 1, 1))
        {
            const ImVec2 UV0(UV.x, UV.y);
            const ImVec2 UV1(UV.z, UV.w);
            ImGui::Image(ID, ImVec2(Size.x, Size.y), UV0, UV1);
        }

        ZY_INLINE void PushStyleColor(ImGuiCol Index, ImVec4 Color)
        {

            ImGui::PushStyleColor(Index, Color);
        }

        ZY_INLINE void PopStyleColor(SInt32 Count = 1)
        {
            ImGui::PopStyleColor(Count);
        }

        ZY_INLINE Bool MenuItem(Text Label, Text Shortcut = {}, Bool Enabled = true)
        {
            return ImGui::MenuItem(Label.GetData(), Shortcut.IsEmpty() ? nullptr : Shortcut.GetData(), false, Enabled);
        }

        ZY_INLINE Bool Button(Text Label, Real32 Width = 0.0f, Real32 Height = 0.0f)
        {
            return ImGui::Button(Label.GetData(), ImVec2(Width, Height));
        }

        ZY_INLINE Bool DisabledButton(Text Label, Bool Disable, Real32 Width = 0.0f, Real32 Height = 0.0f)
        {
            ImGui::BeginDisabled(Disable);

            const Bool Result = Button(Label, Width, Height) && !Disable;

            ImGui::EndDisabled();

            return Result;
        }

        ZY_INLINE Bool SmallButton(Text Label)
        {
            return ImGui::SmallButton(Label.GetData());
        }

        ZY_INLINE Bool Checkbox(Text Label, Ref<Bool> State)
        {
            return ImGui::Checkbox(Label.GetData(), AddressOf(State));
        }

        ZY_INLINE Bool RadioButton(Text Label, Bool Active)
        {
            return ImGui::RadioButton(Label.GetData(), Active);
        }

        ZY_INLINE Bool Selectable(Text Label, Bool Selected = false, ImGuiSelectableFlags Flags = ImGuiSelectableFlags_None)
        {
            return ImGui::Selectable(Label.GetData(), Selected, Flags);
        }

        ZY_INLINE Bool IsItemClicked(ImGuiMouseButton Button = ImGuiMouseButton_Left)
        {
            return ImGui::IsItemClicked(Button);
        }

        ZY_INLINE Bool IsItemToggledOpen() const
        {
            return ImGui::IsItemToggledOpen();
        }

        ZY_INLINE Bool IsItemHovered(ImGuiHoveredFlags Flags = ImGuiHoveredFlags_None) const
        {
            return ImGui::IsItemHovered(Flags);
        }

        ZY_INLINE Bool IsKeyPressed(ImGuiKey Key, Bool Repeat = false) const
        {
            return ImGui::IsKeyPressed(Key, Repeat);
        }

        ZY_INLINE Bool InvisibleButton(Text ID, ImVec2 Size, ImGuiButtonFlags Flags = ImGuiButtonFlags_None)
        {
            return ImGui::InvisibleButton(ID.GetData(), Size, Flags);
        }

        ZY_INLINE Bool IsPopupOpen(Text ID)
        {
            return ImGui::IsPopupOpen(ID.GetData());
        }

        ZY_INLINE void OpenPopup(Text ID, ImGuiPopupFlags Flags = ImGuiPopupFlags_None)
        {
            ImGui::OpenPopup(ID.GetData(), Flags);
        }

        ZY_INLINE Bool BeginPopup(Text ID, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            return ImGui::BeginPopup(ID.GetData(), Flags);
        }

        ZY_INLINE Bool BeginPopupModal(Text ID, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            return ImGui::BeginPopupModal(ID.GetData(), nullptr, Flags);
        }

        ZY_INLINE void CloseCurrentPopup()
        {
            ImGui::CloseCurrentPopup();
        }

        ZY_INLINE Bool BeginPopupContextItem(Text ID = {})
        {
            return ImGui::BeginPopupContextItem(ID.IsEmpty() ? nullptr : ID.GetData());
        }

        ZY_INLINE void EndPopup()
        {
            ImGui::EndPopup();
        }

        ZY_INLINE Bool BeginTabBar(Text ID, ImGuiTabBarFlags Flags = ImGuiTabBarFlags_None)
        {
            return ImGui::BeginTabBar(ID.GetData(), Flags);
        }

        ZY_INLINE void EndTabBar()
        {
            ImGui::EndTabBar();
        }

        ZY_INLINE Bool BeginTabItem(Text Label, ImGuiTabItemFlags Flags = ImGuiTabItemFlags_None)
        {
            return ImGui::BeginTabItem(Label.GetData(), nullptr, Flags);
        }

        ZY_INLINE void EndTabItem()
        {
            ImGui::EndTabItem();
        }

        ZY_INLINE Bool BeginCombo(Text ID, Text Preview, ImGuiComboFlags Flags = ImGuiComboFlags_None)
        {
            return ImGui::BeginCombo(ID.GetData(), Preview.GetData(), Flags);
        }

        ZY_INLINE void EndCombo()
        {
            ImGui::EndCombo();
        }

        ZY_INLINE Bool BeginTable(Text ID, SInt32 Columns, ImGuiTableFlags Flags = ImGuiTableFlags_None, ImVec2 Size = ImVec2(0, 0))
        {
            return ImGui::BeginTable(ID.GetData(), Columns, Flags, Size);
        }

        ZY_INLINE void EndTable()
        {
            ImGui::EndTable();
        }

        ZY_INLINE void TableSetupColumn(Text Label, ImGuiTableColumnFlags Flags = ImGuiTableColumnFlags_None, Real32 InitWidth = 0.0f)
        {
            ImGui::TableSetupColumn(Label.GetData(), Flags, InitWidth);
        }

        ZY_INLINE void TableNextColumn()
        {
            ImGui::TableNextColumn();
        }

        ZY_INLINE void TableHeadersRow()
        {
            ImGui::TableHeadersRow();
        }

        ZY_INLINE void TableNextRow(ImGuiTableRowFlags Flags = ImGuiTableRowFlags_None, Real32 MinHeight = 0.0f)
        {
            ImGui::TableNextRow(Flags, MinHeight);
        }

        ZY_INLINE Bool TableSetColumnIndex(SInt32 Column)
        {
            return ImGui::TableSetColumnIndex(Column);
        }

        ZY_INLINE void CenterCursor(Text Label)
        {
            SetCursorPosX((GetWindowWidth() - CalcTextSize(Label).x) * 0.5f);
        }

        ZY_INLINE Bool InputFloat(Text ID, Ref<Real32> Value, Real32 Step = 0.0f, Real32 StepFast = 0.0f, Text Format = "%.2f", ImGuiInputTextFlags Flags = ImGuiInputTextFlags_None)
        {
            return ImGui::InputFloat(ID.GetData(), AddressOf(Value), Step, StepFast, Format.GetData(), Flags);
        }

        template<typename Type>
        ZY_INLINE Bool InputInt(Text ID, Ref<Type> Value, Type Step = Type(1), Type StepFast = Type(1), ImGuiInputTextFlags Flags = ImGuiInputTextFlags_None)
        {
            SInt32 Temp = static_cast<SInt32>(Value);

            if (ImGui::InputInt(ID.GetData(), AddressOf(Temp), Step, StepFast, Flags))
            {
                Value = static_cast<Type>(Temp);
                return true;
            }
            return false;
        }

        template<typename Type>
        ZY_INLINE Bool InputIntPair(
            Text                ID,
            Ref<Type>           X,
            Ref<Type>           Y,
            Text                Separator = "x",
            Type                Step      = Type(1),
            Type                StepFast  = Type(1),
            ImGuiInputTextFlags Flags     = ImGuiInputTextFlags_None)
        {
            ConstRef<ImGuiStyle> Style = ImGui::GetStyle();

            const Bool   HasSeparator = !Separator.IsEmpty();
            const Real32 Size         = HasSeparator ? ImGui::CalcTextSize(Separator.GetData()).x : 0.0f;
            const Real32 Spacing      = Style.ItemSpacing.x;

            const Real32 Stepper = Step != Type(0) ? (ImGui::GetFrameHeight() + Style.ItemInnerSpacing.x) * 2.0f : 0.0f;
            const Real32 Width   = (ImGui::GetContentRegionAvail().x - Size - Spacing * 2.0f - Stepper * 2.0f) * 0.5f;

            Bool Dirty = false;

            ImGui::SetNextItemWidth(Width);
            Dirty |= InputInt(String<32>::Print<"{0}_x">(ID), X, Step, StepFast, Flags);

            ImGui::SameLine();

            if (HasSeparator)
            {
                PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_TextDisabled]);
                ImGui::TextUnformatted(Separator.GetData(), Separator.GetData() + Separator.GetSize());
                PopStyleColor();

                ImGui::SameLine();
            }

            ImGui::SetNextItemWidth(Width);
            Dirty |= InputInt(String<32>::Print<"{0}_y">(ID), Y, Step, StepFast, Flags);

            return Dirty;
        }

        ZY_INLINE Bool InputFloatAxis(
            Text                ID,
            Text                Tag,
            ImVec4              Accent,
            Ref<Real32>         Value,
            Real32              Width,
            Text                Format = "%.2f",
            Real32              Speed  = 0.1f,
            Real32              Min    = 0.0f,
            Real32              Max    = 0.0f)
        {
            ConstRef<ImGuiStyle> Style = ImGui::GetStyle();

            const ImVec2 TagSize  = CalcTextSize(Tag);
            const Real32 Padding  = Style.FramePadding.x;
            const Real32 TagWidth = TagSize.x + Padding * 2.0f;
            const Real32 Height   = ImGui::GetFrameHeight();
            const ImVec2 Origin   = ImGui::GetCursorScreenPos();

            const ImGuiSliderFlags DragFlags = (Min < Max) ? ImGuiSliderFlags_AlwaysClamp : ImGuiSliderFlags_None;

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(TagWidth + Padding, Style.FramePadding.y));
            ImGui::SetNextItemWidth(Width);
            const Bool Dirty = DragFloat(ID, Value, Speed, Min, Max, Format, DragFlags);
            ImGui::PopStyleVar();

            const Ptr<ImDrawList> Canvas = ImGui::GetWindowDrawList();

            Canvas->AddRectFilled(
                Origin,
                ImVec2(Origin.x + TagWidth, Origin.y + Height),
                ImGui::GetColorU32(Accent),
                Style.FrameRounding,
                ImDrawFlags_RoundCornersLeft);
            Canvas->AddText(
                ImVec2(Origin.x + Padding, Origin.y + (Height - TagSize.y) * 0.5f),
                ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.90f)),
                Tag.GetData(),
                Tag.GetData() + Tag.GetSize());

            return Dirty;
        }

        ZY_INLINE Bool InputFloatPair(
            Text                ID,
            Ref<Real32>         X,
            Ref<Real32>         Y,
            Text                Format = "%.2f",
            Real32              Speed  = 0.1f,
            Real32              Min    = 0.0f,
            Real32              Max    = 0.0f)
        {
            const Real32 Spacing = ImGui::GetStyle().ItemSpacing.x;
            const Real32 Width   = (ImGui::GetContentRegionAvail().x - Spacing) * 0.5f;

            PushID(ID);

            Bool Dirty = InputFloatAxis("##x", "X", kAxisTintX, X, Width, Format, Speed, Min, Max);

            ImGui::SameLine(0.0f, Spacing);

            Dirty |= InputFloatAxis("##y", "Y", kAxisTintY, Y, Width, Format, Speed, Min, Max);

            PopID();

            return Dirty;
        }

        template<typename Callback>
        ZY_INLINE void InputText(
            Text                ID,
            Text                Value,
            AnyRef<Callback>    Action,
            ImGuiInputTextFlags Flags = ImGuiInputTextFlags_None)
        {
            String<1024> Buffer;
            Buffer.Append(Value);

            if (ImGui::InputText(ID.GetData(), Buffer.GetData(), 1024, Flags))
            {
                Action(StrConvert(Buffer.GetData()));
            }
        }

        template<typename InputCallback, typename ButtonCallback>
        ZY_INLINE void InputTextWithButton(
            Text                   ID,
            Text                   TextValue,
            AnyRef<InputCallback>  TextAction,
            Text                   ButtonLabel,
            AnyRef<ButtonCallback> ButtonAction,
            ImGuiInputTextFlags    Flags = ImGuiInputTextFlags_None)
        {
            const Real32 Width = ImGui::CalcTextSize(ButtonLabel.GetData()).x + ImGui::GetStyle().FramePadding.x * 2.0f;
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - Width - ImGui::GetStyle().ItemSpacing.x);
            InputText(ID, TextValue, TextAction, Flags);

            ImGui::SameLine();
            if (ImGui::SmallButton(ButtonLabel.GetData()))
            {
                ButtonAction();
            }
        }

        template<typename Tint>
        ZY_INLINE Bool InputTint(Text ID, Ref<Tint> Value, ImGuiColorEditFlags Flags = ImGuiColorEditFlags_None)
        {
            constexpr Bool IsFloatColor = IsAnyOf<Tint, Color>;

            Real32 RGBA[] = {
                IsFloatColor ? Value.GetRed()   : Value.GetRed()   / 255.0f,
                IsFloatColor ? Value.GetGreen() : Value.GetGreen() / 255.0f,
                IsFloatColor ? Value.GetBlue()  : Value.GetBlue()  / 255.0f,
                IsFloatColor ? Value.GetAlpha() : Value.GetAlpha() / 255.0f
            };

            if (ImGui::ColorEdit4(ID.GetData(), RGBA, Flags))
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
        ZY_INLINE Bool InputTintSmall(Text ID, Ref<Tint> Value)
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

