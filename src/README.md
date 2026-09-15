# 📁 Core Source Driver (`D:\D_v1\src\`)

This folder holds the master assembly timeline files that compile directly into the final native executable binaries.

## 📄 `workspace.asm`

The centralized orchestral conductor script.
1. Establishes the Win64 ABI stack boundaries.
2. Pulls `[Workspace] Width/Height` from `config.ini` via `GetPrivateProfileIntW`, then calls `init_titlebar` (from `titlebar.inc`) to pull `[TitleBar] Location/Thickness/Color` from the same file.
3. Computes global row and byte-size pitch variables in CPU registers.
4. Registers the window class and allocates the frame buffer, then caches the canvas dimensions into `r11`/`r12` for the rest of the paint sequence — every draw macro downstream expects them there.
5. Paints the frame buffer, in order: clear the canvas → draw the title bar (`titlebar.inc`) → compute and stamp the close button (`layout_close_icon` then `draw_icon_masked`, from `titlebar.inc`/`icon.inc`) → the two placeholder squares.
6. Creates the borderless window and hands control to the message pump loop, where `win64_host.inc`'s `WindowProc` takes over for the life of the app.

Include order matters and is commented directly in the file: `icon_ids.inc`/`icon.inc` before `titlebar.inc` (its close-button macros need `ICON_CELL`), and `titlebar.inc` before `win64_host.inc` (its `WM_NCHITTEST`/`WM_LBUTTONDOWN` handlers call straight into `titlebar.inc`'s macros, which have to already be defined by then).

xx

