# 📁 Core Component Libraries (`D:\D_v1\lib\`)

The source files contained in this folder represent modular, single-purpose code components. Most are fully stateless (`draw2d.inc`); a few (`bar.inc`, `icon.inc`) now own a small amount of internal state — their own config values or a layout cache — but each still exposes only a narrow macro interface, so nothing outside the file needs to know how that state is stored or when it's recomputed.

## 📄 `draw2d.inc` (The Painting Kernel)

**Hardware-Agnostic Core Drawing Engine**. It contains zero Windows-specific APIs and zero x86 hardware-specific string instructions (like `rep stosd`).
* **`draw_rect_block` Macro:** Processes register-to-register index bounds arithmetic to inject color signatures cleanly onto raw byte addresses across scanlines. Because it relies entirely on linear array math, this file is fully ready for deployment onto other silicon target variants later.

## 📄 `bar.inc` (The Bar Module)

Owns everything about the draggable window chrome bar — its configuration, its paint pass, and its interactive regions — so nothing else in the codebase needs to know which edge it's docked to.
* **`init_bar` Macro:** Pulls `Location`/`Thickness`/`Color` out of `config.ini`'s `[Bar]` section at boot.
* **`draw_bar` Macro:** Paints the bar along whichever edge is configured — Top, Bottom, Left, or Right — using `draw2d.inc`'s `draw_rect_block` underneath.
* **`hittest_bar` Macro:** Feeds `WM_NCHITTEST` the edge-aware drag region, so `HTCAPTION` fires correctly no matter which side the bar lives on.
* **`layout_close_icon` / `hittest_close_button` Macros:** Position and hit-test the close button — carved out of the general drag region so a click there reaches `WindowProc` as an ordinary click instead of starting a window drag. **Confirmed working on all four edges.**
* **`layout_icon_near_close` / `layout_app_icon` / `layout_center_icons` Macros:** Position minimize/maximize (stacked inward from close), the app icon (opposite end of the bar), and the max-width/max-height pair (centered). Draw-only for now — none of the three have a hit-test yet, so nothing happens if you click them.

## 📄 `icon.inc` (The Tintable Icon Engine)

Owns the embedded symbol atlas and the one routine that turns a flat coverage mask into an on-screen icon in any color, without ever needing a separately-colored source image per color.
* **`symbols_data`:** The full icon sheet (`D:\D_v1\assets\symbols.bin`), embedded directly into the executable at assemble time via FASM's `file` directive — no runtime file I/O, no allocation.
* **`draw_icon_masked` Macro:** Composites a 16×16 coverage mask against a flat tint color, per pixel — `dest = dest*(1-a) + tint*a` — using a fast integer approximation in place of a hardware divide.

## 📄 `icon_ids.inc` (The Icon Index Table)

A flat list of named constants (`ICON_CLOSE`, `ICON_MINIMIZE`, …) mapping each icon's name to its byte offset inside `symbols_data`. Exists because — unlike ASCII glyphs, which get their index for free from their character code — icons have no implicit ordering. This file is the single source of truth that both the atlas-generating tool and every blit/hit-test call site must agree on; nothing should ever reference an icon by a bare numeric index.

## 📄 `win64_host.inc` (The OS Wrapper Layer)

This file handles the **Windows 11 Liaison Tasks**. It isolates the volatile platform elements away from your pure layout logic.
* **Borderless Canvas Hooking:** Implements `WS_POPUP` window registrations to completely drop title bars and window decorations.
* **`WM_NCHITTEST` Infiltration:** Intercepts mouse coordinates and triggers native `HTCAPTION` returns to enable hardware dragging across an abstract dark header row — now deferring to `bar.inc` for the edge-aware bar region, with an explicit close-button exclusion checked first so the button is never swallowed by the drag region.
* **`WM_LBUTTONDOWN` Handling:** Catches ordinary client-area clicks and checks them against the close button's hit region via `bar.inc`'s `hittest_close_button`; a hit calls `DestroyWindow` directly. **Confirmed working.**
* **`StretchDIBits` Presentation:** Executes raw frame buffer bitmap blits directly to the physical graphics card compositor loop during paint refresh tokens.
