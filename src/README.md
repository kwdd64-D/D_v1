# 📁 Core Source Driver (`D:\D_v1\src\`)

This folder holds the master assembly timeline files that compile directly into the final native executable binaries.

## 📄 `workspace.asm`

The centralized orchestral conductor script.
1. Establishes the Win64 ABI stack boundaries.
2. Directs `GetPrivateProfileIntW` to pull configuration text values from your configuration maps — canvas size first, then bar `Location`/`Thickness`/`Color` via `bar.inc`'s `init_bar`.
3. Automatically computes global row and byte size pitch variables in CPU registers.
4. Invokes your decoupled library elements to paint the screen buffer, in order: clear the canvas, paint the bar, position and stamp the close button on top of it, then paint the remaining scene elements — before handing engine control down to the active message processing pump loop.

## 🛠️ Include Order Dependency

`workspace.asm`'s include order isn't arbitrary — several of these files rely on macros defined by files included earlier in the chain:

```
icon_ids.inc   ; constants only, no dependencies
icon.inc       ; no dependencies on the others
bar.inc   ; uses ICON_CELL from icon_ids.inc
win64_host.inc ; invokes bar.inc's hittest_bar / hittest_close_button
draw2d.inc     ; used by bar.inc's draw_bar (only at invocation time)
```

If a new library file introduces a macro that another file's *code* (not just its own macro bodies) invokes directly, it needs to move earlier in this list.
