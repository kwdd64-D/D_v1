# 📁 Core Component Libraries (`D:\D_v1\lib\`)

The source files in this folder are reusable components pulled into `workspace.asm`. Most are hardware/OS-agnostic; one has a deliberate, explicitly-flagged dependency on the Win32 layer — called out per file below rather than glossed over, since that seam is exactly what matters whenever the fasmg migration or a non-Windows target comes up.

## 📄 `draw2d.inc` (The Painting Kernel)
**Hardware-agnostic core drawing engine.** Zero Windows-specific APIs, zero x86 hardware-specific string instructions (like `rep stosd`).
* **`draw_rect_block` Macro:** flat-fills a rectangular region of the frame buffer with a solid color via linear array math. Fully portable to other silicon targets later.

## 📄 `icon.inc` (Tintable Icon Rendering)
**Hardware-agnostic, same standard as `draw2d.inc`.** Zero Windows-specific APIs.
* **`symbols_data`:** the icon atlas, pulled in from `D:\D_v1\assets\symbols.bin` at **assemble time** via FASM's `file` directive — not loaded at runtime, so a missing file is a build-time failure, not a boot-time fallback like `config.ini` gets.
* **`draw_icon_masked` Macro:** composites a single 16×16 coverage mask (format specified in `symbol-atlas-format.md`) into the frame buffer at a flat tint color — `dest = dest*(1-a) + tint*a` per channel — using an integer fast-divide-by-255 identity instead of a hardware divide.

## 📄 `icon_ids.inc` (Icon Name Table)
Pure constants — `ICON_MINIMIZE`, `ICON_CLOSE`, `ICON_MAX_WIDTH`, etc. — mapping each icon's name to its index in `symbols.bin`. No logic, no state. Exists so nothing anywhere ever references an icon by a bare number; appending a new icon means adding one constant here plus one 256-byte cell at the end of the `.bin`, never renumbering.

## 📄 `titlebar.inc` (Title Bar & Close Button)
**Mostly hardware-agnostic, with one deliberate, named exception:** `init_titlebar` calls `GetPrivateProfileIntW` directly — the one real Win32 dependency living inside this file. Everything else here — the bar-drawing math, the drag hit-test math, the close-button layout and hit-test math — is plain arithmetic with no OS calls, same standard as `draw2d.inc`/`icon.inc`.
* **`init_titlebar`:** loads `[TitleBar] Location/Thickness/Color` from `config.ini`.
* **`draw_titlebar`:** paints the bar along whichever edge is configured — Top, Bottom, Left, or Right.
* **`hittest_titlebar`:** the edge-aware drag-region test, called from `win64_host.inc`'s `WM_NCHITTEST` handler.
* **`layout_close_icon` / `hittest_close_button`:** position the close button within the bar (edge-aware, same four cases) and recognize a point landing on it. The *same* macro backs both the drag-exclusion check and the real click handler, so the drawn position and the clickable position can never quietly drift apart from each other.

## 📄 `win64_host.inc` (The OS Wrapper Layer)
This file handles the **Windows Liaison Tasks** — platform glue only, no layout logic of its own anymore.
* **Borderless Canvas Hooking:** `WS_POPUP` window registration, no system title bar or decorations.
* **`WM_NCHITTEST` Dispatch:** delegates the actual geometry to `titlebar.inc` (`hittest_close_button` first, then `hittest_titlebar`) — this file only decides what to do with the answer: return `HTCAPTION` for a drag, or fall through to `DefWindowProc` otherwise.
* **`WM_LBUTTONDOWN` Dispatch:** the close button's real click, checked against the same rectangle `hittest_close_button` excludes from dragging — a hit calls `DestroyWindow`.
* **`StretchDIBits` Presentation:** blits the frame buffer to the window on `WM_PAINT`.


