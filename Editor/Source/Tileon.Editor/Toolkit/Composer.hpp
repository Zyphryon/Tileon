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

#include <ImGuiSymbols.hpp>
#include <ImGuiRenderer.hpp>
#include <Zyphryon.Graphic/Types.hpp>
#include <imgui.h>
#include <imgui_internal.h>

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// [   CODE   ]
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

namespace Tileon::Editor::Toolkit
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
        /// \param Window The title of the window to dock, as passed to \ref Toolkit::Composer::Begin.
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
        static constexpr ImVec4 kAxisTintZ  = ImVec4(0.30f, 0.48f, 0.74f, 1.0f);

    public:

        ZY_INLINE static Bool Begin(Text Title, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            return ImGui::Begin(Title.GetData(), nullptr, Flags);
        }

        ZY_INLINE static Bool Begin(Text Title, Ref<Bool> Open, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            return ImGui::Begin(Title.GetData(), AddressOf(Open), Flags);
        }

        ZY_INLINE static void End()
        {
            ImGui::End();
        }

        ZY_INLINE static Bool IsWindowFocused(ImGuiFocusedFlags Flags = ImGuiFocusedFlags_None)
        {
            return ImGui::IsWindowFocused(Flags);
        }

        ZY_INLINE static void SetWindowFontScale(Real32 Scale)
        {
            ImGui::SetWindowFontScale(Scale);
        }

        ZY_INLINE static void BringWindowToFront()
        {
            ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
        }

        template<typename Function>
        ZY_INLINE static void DockSpace(Text ID, AnyRef<Function> Builder)
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

        ZY_INLINE static void ResetDockSpace(Text ID)
        {
            ImGui::DockBuilderRemoveNode(ImGui::GetID(ID.GetData()));
        }

        ZY_INLINE static void BeginChild(Text ID, ImVec2 Size = ImVec2(0, 0), ImGuiChildFlags ChildFlags = ImGuiChildFlags_None, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            ImGui::BeginChild(ID.GetData(), Size, ChildFlags, Flags);
        }

        ZY_INLINE static void EndChild()
        {
            ImGui::EndChild();
        }

        ZY_INLINE static Bool TreeNode(Text Label, ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_None)
        {
            return ImGui::TreeNodeEx(Label.GetData(), Flags);
        }

        ZY_INLINE static void TreePop()
        {
            ImGui::TreePop();
        }

        ZY_INLINE static void SetNextItemOpen(Bool Open, ImGuiCond Condition = ImGuiCond_None)
        {
            ImGui::SetNextItemOpen(Open, Condition);
        }

        ZY_INLINE static void PushID(Text ID)
        {
            ImGui::PushID(ID.GetData(), ID.GetData() + ID.GetSize());
        }

        ZY_INLINE static void PushID(SInt32 ID)
        {
            ImGui::PushID(ID);
        }

        ZY_INLINE static void PushID(ConstPtr<void> ID)
        {
            ImGui::PushID(ID);
        }

        ZY_INLINE static void PopID()
        {
            ImGui::PopID();
        }

        ZY_INLINE static Bool GetState(Text ID, Bool Fallback = false)
        {
            return ImGui::GetStateStorage()->GetBool(ImGui::GetID(ID.GetData(), ID.GetData() + ID.GetSize()), Fallback);
        }

        ZY_INLINE static void SetState(Text ID, Bool State)
        {
            ImGui::GetStateStorage()->SetBool(ImGui::GetID(ID.GetData(), ID.GetData() + ID.GetSize()), State);
        }

        ZY_INLINE static Bool BeginMainMenuBar()
        {
            return ImGui::BeginMainMenuBar();
        }

        ZY_INLINE static void EndMainMenuBar()
        {
            ImGui::EndMainMenuBar();
        }

        ZY_INLINE static Bool BeginMenu(Text Label, Bool Enabled = true)
        {
            return ImGui::BeginMenu(Label.GetData(), Enabled);
        }

        ZY_INLINE static void EndMenu()
        {
            ImGui::EndMenu();
        }

        ZY_INLINE static ImVec2 GetViewportCenter()
        {
            return ImGui::GetMainViewport()->GetCenter();
        }

        ZY_INLINE static void SetNextWindowPos(ImVec2 Position, ImGuiCond Condition = ImGuiCond_None, ImVec2 Pivot = ImVec2(0.0f, 0.0f))
        {
            ImGui::SetNextWindowPos(Position, Condition, Pivot);
        }

        ZY_INLINE static void SetNextWindowSize(Real32 Width, Real32 Height, ImGuiCond Condition = ImGuiCond_None)
        {
            ImGui::SetNextWindowSize(ImVec2(Width, Height), Condition);
        }

        ZY_INLINE static void SetNextWindowSizeConstraints(Real32 MinWidth, Real32 MinHeight, Real32 MaxWidth = FLT_MAX, Real32 MaxHeight = FLT_MAX)
        {
            ImGui::SetNextWindowSizeConstraints(ImVec2(MinWidth, MinHeight), ImVec2(MaxWidth, MaxHeight));
        }

        ZY_INLINE static void SetNextWindowBgAlpha(Real32 Alpha)
        {
            ImGui::SetNextWindowBgAlpha(Alpha);
        }

        ZY_INLINE static void SetNextWindowFocus()
        {
            ImGui::SetNextWindowFocus();
        }

        ZY_INLINE static ImVec2 GetDisplaySize()
        {
            return ImGui::GetIO().DisplaySize;
        }

        ZY_INLINE static Real32 GetWindowBottom(UInt32 Rows = 1)
        {
            return ImGui::GetWindowHeight()
                - ImGui::GetFrameHeightWithSpacing() * static_cast<Real32>(Rows)
                - ImGui::GetStyle().WindowPadding.y;
        }

        ZY_INLINE static void SliderFloat(Text ID, Ref<Real32> Value, Real32 Min, Real32 Max, Text Format = "%.2f")
        {
            ImGui::SliderFloat(ID.GetData(), AddressOf(Value), Min, Max, Format.GetData());
        }

        ZY_INLINE static Bool DragFloat(
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

        ZY_INLINE static Bool DragInt(
            Text             ID,
            Ref<SInt32>      Value,
            Real32           Speed  = 1.0f,
            SInt32           Min    = 0,
            SInt32           Max    = 0,
            Text             Format = "%d",
            ImGuiSliderFlags Flags  = ImGuiSliderFlags_None)
        {
            return ImGui::DragInt(ID.GetData(), AddressOf(Value), Speed, Min, Max, Format.GetData(), Flags);
        }

        ZY_INLINE static Bool SliderAngle(Text ID, Ref<Real32> Radians, Real32 Min = 0.0f, Real32 Max = 360.0f, Text Format = "%.1f°")
        {
            return ImGui::SliderAngle(ID.GetData(), AddressOf(Radians), Min, Max, Format.GetData());
        }

        ZY_INLINE static void BeginTooltip()
        {
            ImGui::BeginTooltip();
        }

        ZY_INLINE static void EndTooltip()
        {
            ImGui::EndTooltip();
        }

        ZY_INLINE static void SetTooltip(Text Text)
        {
            if (ImGui::BeginTooltipEx(ImGuiTooltipFlags_OverridePrevious, ImGuiWindowFlags_None))
            {
                ImGui::TextUnformatted(Text.GetData(), Text.GetData() + Text.GetSize());
                ImGui::EndTooltip();
            }
        }

        ZY_INLINE static void Tooltip(Text Text)
        {
            if (!Text.IsEmpty() && IsItemHovered(ImGuiHoveredFlags_ForTooltip))
            {
                SetTooltip(Text);
            }
        }

        ZY_INLINE static void SameLine(Real32 OffsetX = 0.0f, Real32 Spacing = -1.0f)
        {
            ImGui::SameLine(OffsetX, Spacing);
        }

        ZY_INLINE static void Spacing()
        {
            ImGui::Spacing();
        }

        ZY_INLINE static void Separator()
        {
            ImGui::Separator();
        }

        ZY_INLINE static void SeparatorEx(ImGuiSeparatorFlags Flags = ImGuiSeparatorFlags_None)
        {
            ImGui::SeparatorEx(Flags);
        }

        ZY_INLINE static ImVec2 GetContentRegionAvail()
        {
            return ImGui::GetContentRegionAvail();
        }

        ZY_INLINE static ImVec2 GetCursorScreenPos()
        {
            return ImGui::GetCursorScreenPos();
        }

        ZY_INLINE static Real32 GetFrameHeight()
        {
            return ImGui::GetFrameHeight();
        }

        ZY_INLINE static Real32 GetFrameHeightWithSpacing()
        {
            return ImGui::GetFrameHeightWithSpacing();
        }

        ZY_INLINE static Real32 GetTextLineHeight()
        {
            return ImGui::GetTextLineHeight();
        }

        ZY_INLINE static void SetCursorScreenPos(ImVec2 Pos)
        {
            ImGui::SetCursorScreenPos(Pos);
        }

        ZY_INLINE static ImVec2 CalcTextSize(Text Text)
        {
            return ImGui::CalcTextSize(Text.GetData(), Text.GetData() + Text.GetSize());
        }

        ZY_INLINE static void PushItemWidth(Real32 Width)
        {
            ImGui::PushItemWidth(Width);
        }

        ZY_INLINE static void PopItemWidth()
        {
            ImGui::PopItemWidth();
        }

        ZY_INLINE static void SetNextItemWidth(Real32 Width)
        {
            ImGui::SetNextItemWidth(Width);
        }

        ZY_INLINE static void SetCursorPosX(Real32 X)
        {
            ImGui::SetCursorPosX(X);
        }

        ZY_INLINE static void SetCursorPosY(Real32 Y)
        {
            ImGui::SetCursorPosY(Y);
        }

        ZY_INLINE static void SetScrollHereY(Real32 CenterRatio = 0.5f)
        {
            ImGui::SetScrollHereY(CenterRatio);
        }

        ZY_INLINE static ImVec2 GetItemRectMin()
        {
            return ImGui::GetItemRectMin();
        }

        ZY_INLINE static ImVec2 GetItemRectSize()
        {
            return ImGui::GetItemRectSize();
        }

        ZY_INLINE static ImVec2 GetMousePos()
        {
            return ImGui::GetMousePos();
        }

        ZY_INLINE static ImVec2 GetMouseDelta()
        {
            return ImGui::GetIO().MouseDelta;
        }

        ZY_INLINE static Real32 GetMouseWheel()
        {
            return ImGui::GetIO().MouseWheel;
        }

        ZY_INLINE static Bool IsMouseClicked(ImGuiMouseButton Button = ImGuiMouseButton_Left, Bool Repeat = false)
        {
            return ImGui::IsMouseClicked(Button, Repeat);
        }

        ZY_INLINE static Bool IsMouseDragging(ImGuiMouseButton Button = ImGuiMouseButton_Left, Real32 LockThreshold = -1.0f)
        {
            return ImGui::IsMouseDragging(Button, LockThreshold);
        }

        ZY_INLINE static Bool IsMouseHoveringRect(ImVec2 Min, ImVec2 Max, Bool Clip = true)
        {
            return ImGui::IsMouseHoveringRect(Min, Max, Clip);
        }

        ZY_INLINE static Bool IsMouseDown(ImGuiMouseButton Button = ImGuiMouseButton_Left)
        {
            return ImGui::IsMouseDown(Button);
        }

        ZY_INLINE static Bool IsMouseReleased(ImGuiMouseButton Button = ImGuiMouseButton_Left)
        {
            return ImGui::IsMouseReleased(Button);
        }

        ZY_INLINE static void SetMouseCursor(ImGuiMouseCursor Cursor)
        {
            ImGui::SetMouseCursor(Cursor);
        }

        ZY_INLINE static Real64 GetTime()
        {
            return ImGui::GetTime();
        }

        ZY_INLINE static Real32 GetDeltaTime()
        {
            return ImGui::GetIO().DeltaTime;
        }

        ZY_INLINE static Bool IsTextInputActive()
        {
            return ImGui::GetIO().WantTextInput;
        }

        ZY_INLINE static ConstRef<ImGuiStyle> GetStyle()
        {
            return ImGui::GetStyle();
        }

        ZY_INLINE static ImU32 GetColorU32(ImGuiCol Index)
        {
            return ImGui::GetColorU32(Index);
        }

        ZY_INLINE static ImU32 GetColorU32(ImGuiCol Index, Real32 Alpha)
        {
            return ImGui::GetColorU32(Index, Alpha);
        }

        ZY_INLINE static void PushStyleVar(ImGuiStyleVar Index, Real32 Value)
        {
            ImGui::PushStyleVar(Index, Value);
        }

        ZY_INLINE static void PushStyleVar(ImGuiStyleVar Index, ImVec2 Value)
        {
            ImGui::PushStyleVar(Index, Value);
        }

        ZY_INLINE static void PopStyleVar(SInt32 Count = 1)
        {
            ImGui::PopStyleVar(Count);
        }

        ZY_INLINE static ImVec4 GetStyleColorVec4(ImGuiCol Index)
        {
            return ImGui::GetStyleColorVec4(Index);
        }

        ZY_INLINE static Ptr<ImDrawList> GetWindowDrawList()
        {
            return ImGui::GetWindowDrawList();
        }

        ZY_INLINE static Ptr<ImDrawList> GetBackgroundDrawList()
        {
            return ImGui::GetBackgroundDrawList();
        }

        ZY_INLINE static void BeginDisabled(Bool Disabled = true)
        {
            ImGui::BeginDisabled(Disabled);
        }

        ZY_INLINE static void EndDisabled()
        {
            ImGui::EndDisabled();
        }

        ZY_INLINE static Bool IsDisabled()
        {
            return (ImGui::GetCurrentContext()->CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
        }

        ZY_INLINE static void Header(Text Label)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.70f, 0.85f, 1.0f));
            ImGui::SeparatorTextEx(0, Label.GetData(), Label.GetData() + Label.GetSize(), 0.0f);
            ImGui::PopStyleColor();
        }

        ZY_INLINE static void Section(Text Label)
        {
            Header(Label);
            Spacing();
        }

        ZY_INLINE static void Field(Text Label)
        {
            PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::TextUnformatted(Label.GetData(), Label.GetData() + Label.GetSize());
            PopStyleColor();
        }

        ZY_INLINE static void FieldInline(Text Label, Real32 LabelWidth = 110.0f)
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

        ZY_INLINE static void Indent(Real32 Width = 0.0f)
        {
            ImGui::Indent(Width);
        }

        ZY_INLINE static void Unindent(Real32 Width = 0.0f)
        {
            ImGui::Unindent(Width);
        }

        ZY_INLINE static void TextUnformatted(Text Formatted)
        {
            ImGui::TextUnformatted(Formatted.GetData(), Formatted.GetData() + Formatted.GetSize());
        }

        ZY_INLINE static void Label(Text Formatted)
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(Formatted.GetData(), Formatted.GetData() + Formatted.GetSize());
        }

        template<typename... Arguments>
        ZY_INLINE static void Label(Text Format, AnyRef<Arguments>... Parameters)
        {
            ImGui::AlignTextToFramePadding();

            String<1024> Text;
            Text.Format(Format::Pattern(Format), Parameters...);
            ImGui::TextUnformatted(Text.GetData(), Text.GetData() + Text.GetSize());
        }

        ZY_INLINE static void TextDisabled(Text Formatted)
        {
            PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::TextUnformatted(Formatted.GetData(), Formatted.GetData() + Formatted.GetSize());
            PopStyleColor();
        }

        template<typename... Arguments>
        ZY_INLINE static void TextDisabled(Text Format, AnyRef<Arguments>... Parameters)
        {
            String<1024> Text;
            Text.Format(Format::Pattern(Format), Parameters...);

            PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::TextUnformatted(Text.GetData(), Text.GetData() + Text.GetSize());
            PopStyleColor();
        }

        ZY_INLINE static void TextColored(ConstRef<ImVec4> Color, Text Formatted)
        {
            PushStyleColor(ImGuiCol_Text, Color);
            ImGui::TextUnformatted(Formatted.GetData(), Formatted.GetData() + Formatted.GetSize());
            PopStyleColor();
        }

        template<typename... Arguments>
        ZY_INLINE static void TextColored(ConstRef<ImVec4> Color, Text Format, AnyRef<Arguments>... Parameters)
        {
            String<1024> Text;
            Text.Format(Format::Pattern(Format), Parameters...);

            PushStyleColor(ImGuiCol_Text, Color);
            ImGui::TextUnformatted(Text.GetData(), Text.GetData() + Text.GetSize());
            PopStyleColor();
        }

        ZY_INLINE static Real32 GetWindowWidth()
        {
            return ImGui::GetWindowWidth();
        }

        ZY_INLINE static Real32 GetWindowHeight()
        {
            return ImGui::GetWindowHeight();
        }

        ZY_INLINE static Real32 GetCursorPosX()
        {
            return ImGui::GetCursorPosX();
        }

        ZY_INLINE static Real32 GetCursorPosY()
        {
            return ImGui::GetCursorPosY();
        }

        ZY_INLINE static void BeginGroup()
        {
            ImGui::BeginGroup();
        }

        ZY_INLINE static void EndGroup()
        {
            ImGui::EndGroup();
        }

        ZY_INLINE static void Image(ImTextureID ID, ImVec2 Size, ImVec4 UV = ImVec4(0, 0, 1, 1))
        {
            const ImVec2 UV0(UV.x, UV.y);
            const ImVec2 UV1(UV.z, UV.w);
            ImGui::Image(ID, ImVec2(Size.x, Size.y), UV0, UV1);
        }

        ZY_INLINE static void PushStyleColor(ImGuiCol Index, ImVec4 Color)
        {

            ImGui::PushStyleColor(Index, Color);
        }

        ZY_INLINE static void PopStyleColor(SInt32 Count = 1)
        {
            ImGui::PopStyleColor(Count);
        }

        ZY_INLINE static Bool MenuItem(Text Label, Text Shortcut = {}, Bool Enabled = true)
        {
            return ImGui::MenuItem(Label.GetData(), Shortcut.IsEmpty() ? nullptr : Shortcut.GetData(), false, Enabled);
        }

        ZY_INLINE static Bool Button(Text Label, Real32 Width = 0.0f, Real32 Height = 0.0f)
        {
            return ImGui::Button(Label.GetData(), ImVec2(Width, Height));
        }

        ZY_INLINE static Bool DisabledButton(Text Label, Bool Disable, Real32 Width = 0.0f, Real32 Height = 0.0f)
        {
            ImGui::BeginDisabled(Disable);

            const Bool Result = Button(Label, Width, Height) && !Disable;

            ImGui::EndDisabled();

            return Result;
        }

        ZY_INLINE static Bool SmallButton(Text Label)
        {
            return ImGui::SmallButton(Label.GetData());
        }

        ZY_INLINE static Bool Checkbox(Text Label, Ref<Bool> State)
        {
            return ImGui::Checkbox(Label.GetData(), AddressOf(State));
        }

        ZY_INLINE static Bool RadioButton(Text Label, Bool Active)
        {
            return ImGui::RadioButton(Label.GetData(), Active);
        }

        ZY_INLINE static Bool Selectable(Text Label, Bool Selected = false, ImGuiSelectableFlags Flags = ImGuiSelectableFlags_None)
        {
            return ImGui::Selectable(Label.GetData(), Selected, Flags);
        }

        ZY_INLINE static Bool BeginDragDropSource(ImGuiDragDropFlags Flags = ImGuiDragDropFlags_None)
        {
            return ImGui::BeginDragDropSource(Flags);
        }

        template<typename Type>
        ZY_INLINE static void SetDragDropPayload(Text ID, ConstRef<Type> Payload)
        {
            ImGui::SetDragDropPayload(ID.GetData(), AddressOf(Payload), sizeof(Type));
        }

        ZY_INLINE static void EndDragDropSource()
        {
            ImGui::EndDragDropSource();
        }

        ZY_INLINE static Bool BeginDragDropTarget()
        {
            return ImGui::BeginDragDropTarget();
        }

        template<typename Type>
        ZY_INLINE static ConstPtr<Type> AcceptDragDropPayload(Text ID)
        {
            const ConstPtr<ImGuiPayload> Data = ImGui::AcceptDragDropPayload(ID.GetData());
            return Data ? static_cast<ConstPtr<Type>>(Data->Data) : nullptr;
        }

        ZY_INLINE static void EndDragDropTarget()
        {
            ImGui::EndDragDropTarget();
        }

        ZY_INLINE static Bool IsItemClicked(ImGuiMouseButton Button = ImGuiMouseButton_Left)
        {
            return ImGui::IsItemClicked(Button);
        }

        ZY_INLINE static Bool IsItemToggledOpen()
        {
            return ImGui::IsItemToggledOpen();
        }

        ZY_INLINE static Bool IsItemActive()
        {
            return ImGui::IsItemActive();
        }

        ZY_INLINE static void SetItemDefaultFocus()
        {
            ImGui::SetItemDefaultFocus();
        }

        ZY_INLINE static Bool IsItemHovered(ImGuiHoveredFlags Flags = ImGuiHoveredFlags_None)
        {
            return ImGui::IsItemHovered(Flags);
        }

        ZY_INLINE static Bool IsItemDeactivatedAfterEdit()
        {
            return ImGui::IsItemDeactivatedAfterEdit();
        }

        ZY_INLINE static Bool IsKeyPressed(ImGuiKey Key, Bool Repeat = false)
        {
            return ImGui::IsKeyPressed(Key, Repeat);
        }

        ZY_INLINE static Bool IsKeyDown(ImGuiKey Key)
        {
            return ImGui::IsKeyDown(Key);
        }

        ZY_INLINE static Bool InvisibleButton(Text ID, ImVec2 Size, ImGuiButtonFlags Flags = ImGuiButtonFlags_None)
        {
            return ImGui::InvisibleButton(ID.GetData(), Size, Flags);
        }

        ZY_INLINE static Bool IsRectVisible(ImVec2 Size)
        {
            return ImGui::IsRectVisible(Size);
        }

        ZY_INLINE static void Dummy(ImVec2 Size)
        {
            ImGui::Dummy(Size);
        }

        ZY_INLINE static Bool IsPopupOpen(Text ID)
        {
            return ImGui::IsPopupOpen(ID.GetData());
        }

        ZY_INLINE static void OpenPopup(Text ID, ImGuiPopupFlags Flags = ImGuiPopupFlags_None)
        {
            ImGui::OpenPopup(ID.GetData(), Flags);
        }

        ZY_INLINE static Bool BeginPopup(Text ID, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            return ImGui::BeginPopup(ID.GetData(), Flags);
        }

        ZY_INLINE static Bool BeginPopupModal(Text ID, ImGuiWindowFlags Flags = ImGuiWindowFlags_None)
        {
            return ImGui::BeginPopupModal(ID.GetData(), nullptr, Flags);
        }

        ZY_INLINE static void CloseCurrentPopup()
        {
            ImGui::CloseCurrentPopup();
        }

        ZY_INLINE static Bool BeginPopupContextItem(Text ID = {})
        {
            return ImGui::BeginPopupContextItem(ID.IsEmpty() ? nullptr : ID.GetData());
        }

        ZY_INLINE static Bool BeginPopupContextWindow(
            Text ID = {}, ImGuiPopupFlags Flags = ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)
        {
            return ImGui::BeginPopupContextWindow(ID.IsEmpty() ? nullptr : ID.GetData(), Flags);
        }

        ZY_INLINE static void EndPopup()
        {
            ImGui::EndPopup();
        }

        ZY_INLINE static Bool BeginTabBar(Text ID, ImGuiTabBarFlags Flags = ImGuiTabBarFlags_None)
        {
            return ImGui::BeginTabBar(ID.GetData(), Flags);
        }

        ZY_INLINE static void EndTabBar()
        {
            ImGui::EndTabBar();
        }

        ZY_INLINE static Bool BeginTabItem(Text Label, ImGuiTabItemFlags Flags = ImGuiTabItemFlags_None)
        {
            return ImGui::BeginTabItem(Label.GetData(), nullptr, Flags);
        }

        ZY_INLINE static void EndTabItem()
        {
            ImGui::EndTabItem();
        }

        ZY_INLINE static Bool BeginCombo(Text ID, Text Preview, ImGuiComboFlags Flags = ImGuiComboFlags_None)
        {
            return ImGui::BeginCombo(ID.GetData(), Preview.GetData(), Flags);
        }

        ZY_INLINE static void EndCombo()
        {
            ImGui::EndCombo();
        }

        template<IsEnum Type>
        ZY_INLINE static Bool Combo(Text ID, Ref<Type> Value)
        {
            Bool Dirty = false;

            if (BeginCombo(ID, Enum::GetName(Value)))
            {
                for (const Type Option : Enum::GetValues<Type>())
                {
                    const Bool Selected = (Option == Value);

                    if (Selectable(Enum::GetName(Option), Selected))
                    {
                        Value = Option;
                        Dirty = true;
                    }

                    if (Selected)
                    {
                        SetItemDefaultFocus();
                    }
                }
                EndCombo();
            }
            return Dirty;
        }

        ZY_INLINE static Bool BeginTable(Text ID, SInt32 Columns, ImGuiTableFlags Flags = ImGuiTableFlags_None, ImVec2 Size = ImVec2(0, 0))
        {
            return ImGui::BeginTable(ID.GetData(), Columns, Flags, Size);
        }

        ZY_INLINE static void EndTable()
        {
            ImGui::EndTable();
        }

        ZY_INLINE static void TableSetupColumn(Text Label, ImGuiTableColumnFlags Flags = ImGuiTableColumnFlags_None, Real32 InitWidth = 0.0f)
        {
            ImGui::TableSetupColumn(Label.GetData(), Flags, InitWidth);
        }

        ZY_INLINE static void TableNextColumn()
        {
            ImGui::TableNextColumn();
        }

        ZY_INLINE static void TableHeadersRow()
        {
            ImGui::TableHeadersRow();
        }

        ZY_INLINE static void TableNextRow(ImGuiTableRowFlags Flags = ImGuiTableRowFlags_None, Real32 MinHeight = 0.0f)
        {
            ImGui::TableNextRow(Flags, MinHeight);
        }

        ZY_INLINE static Bool TableSetColumnIndex(SInt32 Column)
        {
            return ImGui::TableSetColumnIndex(Column);
        }

        ZY_INLINE static void CenterCursor(Text Label)
        {
            SetCursorPosX((GetWindowWidth() - CalcTextSize(Label).x) * 0.5f);
        }

        ZY_INLINE static Bool InputFloat(Text ID, Ref<Real32> Value, Real32 Step = 0.0f, Real32 StepFast = 0.0f, Text Format = "%.2f", ImGuiInputTextFlags Flags = ImGuiInputTextFlags_None)
        {
            return ImGui::InputFloat(ID.GetData(), AddressOf(Value), Step, StepFast, Format.GetData(), Flags);
        }

        template<typename Type>
        ZY_INLINE static Bool InputInt(Text ID, Ref<Type> Value, Type Step = Type(1), Type StepFast = Type(1), ImGuiInputTextFlags Flags = ImGuiInputTextFlags_None)
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
        ZY_INLINE static Bool InputIntPair(
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

        ZY_INLINE static Bool InputFloatAxis(
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

        ZY_INLINE static Bool InputFloatPair(
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

        ZY_INLINE static Bool InputFloatTriple(
            Text                ID,
            Ref<Real32>         X,
            Ref<Real32>         Y,
            Ref<Real32>         Z,
            Text                Format = "%.2f",
            Real32              Speed  = 0.1f,
            Real32              Min    = 0.0f,
            Real32              Max    = 0.0f)
        {
            const Real32 Spacing = ImGui::GetStyle().ItemSpacing.x;
            const Real32 Width   = (ImGui::GetContentRegionAvail().x - Spacing * 2.0f) / 3.0f;

            PushID(ID);

            Bool Dirty = InputFloatAxis("##x", "X", kAxisTintX, X, Width, Format, Speed, Min, Max);

            ImGui::SameLine(0.0f, Spacing);

            Dirty |= InputFloatAxis("##y", "Y", kAxisTintY, Y, Width, Format, Speed, Min, Max);

            ImGui::SameLine(0.0f, Spacing);

            Dirty |= InputFloatAxis("##z", "Z", kAxisTintZ, Z, Width, Format, Speed, Min, Max);

            PopID();

            return Dirty;
        }

        template<typename Callback>
        ZY_INLINE static void InputText(
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

        /// \brief Draws a text field followed by two small buttons.
        ///
        /// \param ID             The identifier of the field.
        /// \param TextValue      The text the field shows.
        /// \param TextAction     The callable invoked with the text the field commits.
        /// \param FirstLabel     The label of the first button.
        /// \param FirstAction    The callable invoked when the first button is pressed.
        /// \param SecondLabel    The label of the second button.
        /// \param SecondDisabled Whether the second button rejects input.
        /// \param SecondAction   The callable invoked when the second button is pressed.
        /// \param Flags          The flags applied to the text field.
        template<typename InputCallback, typename FirstCallback, typename SecondCallback>
        ZY_INLINE static void InputTextWithButtons(
            Text                   ID,
            Text                   TextValue,
            AnyRef<InputCallback>  TextAction,
            Text                   FirstLabel,
            AnyRef<FirstCallback>  FirstAction,
            Text                   SecondLabel,
            Bool                   SecondDisabled,
            AnyRef<SecondCallback> SecondAction,
            ImGuiInputTextFlags    Flags = ImGuiInputTextFlags_None)
        {
            ConstRef<ImGuiStyle> Style = ImGui::GetStyle();

            const Real32 FirstWidth  = ImGui::CalcTextSize(FirstLabel.GetData()).x  + Style.FramePadding.x * 2.0f;
            const Real32 SecondWidth = ImGui::CalcTextSize(SecondLabel.GetData()).x + Style.FramePadding.x * 2.0f;

            ImGui::SetNextItemWidth(
                ImGui::GetContentRegionAvail().x - FirstWidth - SecondWidth - Style.ItemSpacing.x * 2.0f);
            InputText(ID, TextValue, TextAction, Flags);

            ImGui::SameLine();
            ImGui::PushID(ID.GetData());

            if (ImGui::SmallButton(FirstLabel.GetData()))
            {
                FirstAction();
            }

            ImGui::SameLine();
            ImGui::BeginDisabled(SecondDisabled);

            if (ImGui::SmallButton(SecondLabel.GetData()) && !SecondDisabled)
            {
                SecondAction();
            }
            ImGui::EndDisabled();
            ImGui::PopID();
        }

        template<typename InputCallback, typename ButtonCallback>
        ZY_INLINE static void InputTextWithButton(
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
            ImGui::PushID(ID.GetData());

            if (ImGui::SmallButton(ButtonLabel.GetData()))
            {
                ButtonAction();
            }

            ImGui::PopID();
        }

        template<typename Tint>
        ZY_INLINE static Bool InputTint(Text ID, Ref<Tint> Value, ImGuiColorEditFlags Flags = ImGuiColorEditFlags_None)
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
        ZY_INLINE static Bool InputTintSmall(Text ID, Ref<Tint> Value)
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