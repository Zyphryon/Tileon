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
| **Hold Space + LMB drag** | Pan the view (the cursor turns into a hand while Space is held) |
| **MMB drag** | Pan the view |
| **LMB drag** (Hand brush) | Pan the view |
| **Alt + LMB drag** | Swing the view — yaw and tilt. **Axonometric projection only** |
| **Projection combo** | Ortho · Isometric · Axonometric |
| **Toolbar X / Y fields** | Type a world coordinate, then **Enter** or click away to jump the camera there |
| **🔍➕ / 🔍➖ / 🏠 buttons** | Zoom in · zoom out · reset zoom |

The camera position is remembered per project — reopening a project restores the last view.

Swinging exists because a 3D shape read straight on is just a rectangle: a few degrees of yaw is
what separates its depth from its height. It does nothing in Ortho or Isometric, where the angle
is fixed.

---

## 🧭 Viewport — Select & Transform

*Select brush (arrow) — the gizmo appears on the current selection.*

| Input | Action |
|-------|--------|
| **LMB** | Select the entity under the cursor |
| **Shift + LMB** | Add / remove that entity from the selection |
| **LMB drag** | Marquee box-select |
| **Shift + LMB drag** | Add the marquee result to the selection |
| **Q / W / E** | Gizmo mode → Move · Rotate · Scale |
| **Drag a gizmo handle** | Move / rotate / scale the whole selection |
| **Ctrl + V** | Paste at the cursor |

Whether a press is a click or a marquee is decided on release, by how far you dragged.

The gizmo's handles sit on the **primary** entity's origin, and that origin is the pivot the whole
selection turns and scales about.

> ⚠️ A gizmo drag cannot be cancelled once started, and the editor has no undo — release only when
> the result is what you want.

### Clipboard & delete

*These act on the whole selection whenever the **viewport is focused**, whichever brush is active.*

| Input | Action |
|-------|--------|
| **Ctrl + C** | Copy selection |
| **Ctrl + X** | Cut selection |
| **Delete** | Delete selection |

---

## 🖌️ Viewport — Tile Mode

| Input | Action |
|-------|--------|
| **LMB / LMB drag** | Paint the selected tile (Pencil paints a continuous stroke) |
| **RMB / RMB drag** | Erase |
| **Bucket brush** | Fill the whole region |
| **🧲 Magnet toggle** | Grid-aligned (seamless) ⇄ free (precise) stamping |
| **Layer 1 / 2 buttons** | Paint on the Base or Detail layer |

The floating footprint shows exactly which cells — and which tile art — will be placed.

---

## 🧱 Viewport — Entity Mode

*Placement happens with the Select brush, with an archetype picked in the Palette.*

| Input | Action |
|-------|--------|
| **LMB** | Place the selected archetype |
| **RMB** | Remove the entity under the cursor |
| **Hold Q / E** | Rotate the pending entity (hold **Shift** for fine control) |
| **Ctrl + Mouse wheel** | Scale the pending entity |
| **✛ Center toggle, or hold Shift** | Snap placement to the tile's center |

> **Note:** Q and E do double duty. Tapping either also switches the gizmo's mode, which is
> harmless while a placement is pending because the gizmo is not on screen.

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

## 🔍 Inspector

| Input | Action |
|-------|--------|
| **Asset field → …** | Browse for an asset |
| **Asset field → ⟳** | Reload the asset from disk (disabled until it has finished loading) |
| **Type a path + Enter** | Assign an asset by path |

Reloading re-reads the **baked** file. A material pointing at a `.tex` refreshes after the baker
runs, not after you edit the source art.

---

## 🔬 Preview Panes

*The image preview in the Foundry and Archetypes side panels. These act on **hover**, not focus.*

| Input | Action |
|-------|--------|
| **Mouse wheel** | Zoom, keeping the point under the cursor fixed |
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

> **Note:** *Save* is invoked from the **File** menu; the `Ctrl+S` text is the menu's shortcut
> label, not a bound key.
