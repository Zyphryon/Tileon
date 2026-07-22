# 🎛️ Tileon Editor — Hotkeys & Input

A quick reference for every keyboard and mouse interaction available in the Tileon Editor.

Conventions: **LMB** = left mouse button, **RMB** = right, **MMB** = middle. "Viewport" is the
main world view (the **Atelier** panel). Most viewport keys require the viewport to be focused
(click it once). Keys never fire while you are typing in a text field.

---

## 🗺️ Viewport — Navigation

*Works in any mode, with any brush.*

| Input | Action |
|-------|--------|
| **Mouse wheel** | Zoom in / out, centered on the cursor |
| **Hold Space + LMB drag** | Pan the view |
| **MMB drag** | Pan the view |
| **Toolbar X / Y fields** | Type a world coordinate, then **Enter** or click away to jump the camera there |
| **🔍➕ / 🔍➖ / 🏠 buttons** | Zoom in · zoom out · reset zoom |

The camera position is remembered per project — reopening a project restores the last view.

---

## 🧭 Viewport — Select & Transform

*Select tool (arrow) — the gizmo appears on the current selection.*

| Input | Action |
|-------|--------|
| **LMB** | Select the entity under the cursor |
| **Shift + LMB** | Add / remove that entity from the selection |
| **LMB drag** | Marquee box-select |
| **Shift + LMB drag** | Add the marquee result to the selection |
| **Q / W / E** | Gizmo mode → Move · Rotate · Scale |
| **Drag a gizmo handle** | Move / rotate / scale the whole selection |
| **Ctrl + C** | Copy selection |
| **Ctrl + X** | Cut selection |
| **Ctrl + V** | Paste at the cursor |
| **Delete** | Delete selection |

---

## 🖌️ Viewport — Tile Mode

| Input | Action |
|-------|--------|
| **LMB / LMB drag** | Paint the selected tile (Pencil paints a continuous stroke) |
| **RMB / RMB drag** | Erase |
| **Bucket brush** | Fill the whole region |
| **🧲 Magnet toggle** | Grid-aligned (seamless) ⇄ free (precise) stamping |
| **Layer 1 / 2** | Paint on the Base or Detail layer |

The floating footprint shows exactly which cells — and which tile art — will be placed.

---

## 🧱 Viewport — Entity Mode

| Input | Action |
|-------|--------|
| **LMB** | Place the selected archetype |
| **RMB** | Remove the entity under the cursor |
| **Hold Q / E** | Rotate the pending entity (hold **Shift** for fine control) |
| **Ctrl + Mouse wheel** | Scale the pending entity |
| **✛ Center toggle, or hold Shift** | Snap placement to the tile's center |

---

## 🎨 Palette

| Input | Action |
|-------|--------|
| **LMB** | Pick the terrain / entity to paint or place |
| **RMB on a Terrain** | Jump to the **Foundry** editor with it selected |
| **RMB on an Entity** | Jump to the **Archetypes** editor with it selected |

---

## 🌳 Hierarchy

| Input | Action |
|-------|--------|
| **LMB** | Select an entity |
| **Delete** | Remove the selected entity (panel focused) |
| **RMB** | Context menu |

---

## 🧬 Archetypes

| Input | Action |
|-------|--------|
| **LMB** | Select an archetype |
| **Drag node → node** | Re-parent, building prefab hierarchies by hand |
| **RMB** | Context menu → New Child · Detach · Delete · Refresh |

The component list's **RMB** menu offers Override / Remove / Revert to Archetype.

---

## 🏭 Foundry (Terrain editor)

| Input | Action |
|-------|--------|
| **LMB** | Select / edit a terrain |
| **RMB** | Context menu → Clone · Delete |

---

## 🔬 Preview Panes

*The image preview in the Foundry and Archetypes side panels.*

| Input | Action |
|-------|--------|
| **Mouse wheel** | Zoom |
| **MMB drag** | Pan |
| **R** | Reset the preview view |

---

## 📋 Menu Bar

| Menu | Item | Action |
|------|------|--------|
| **File** | Save *(Ctrl+S)* | Save the project / world |
| **File** | Exit | Quit the editor |
| **View** | *(panel names)* | Toggle each panel's visibility |
| **View** | Reset Layout | Restore the default dock arrangement |
| **Settings** | Fullscreen | Toggle fullscreen |
| **Help** | Theme → Dark / Light | Switch the UI theme |

> **Note:** *Save* is invoked from the **File** menu; the `Ctrl+S` text is the menu's shortcut label.
