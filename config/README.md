# 📁 Configuration Layer (`D:\D_v1\config\`)

This directory houses the human-readable text configuration scripts that determine your workspace and title bar parameters at initialization without requiring recompilation.

## 📄 `config.ini` Specifications
The main settings script uses standard Windows `.ini` structuring, read at **runtime** via `GetPrivateProfileIntW` — nothing in this file is compiled in, so a change takes effect on the next launch with no rebuild.

```ini
[Workspace]
Width=1024        ; Dynamic width of the frame buffer canvas in pixels
Height=768         ; Dynamic height of the frame buffer canvas in pixels

[TitleBar]
Location=0         ; 0=Top 1=Bottom 2=Left 3=Right
Thickness=40       ; Bar thickness in pixels
Color=0xFF313244   ; 0xRRGGBB — hex accepted via the "0x" prefix
```

## 🛠️ Execution Mechanics
At boot time, `workspace.asm` pulls `[Workspace] Width/Height` directly, then calls `init_titlebar` (in `D:\D_v1\lib\titlebar.inc`) to pull the `[TitleBar]` keys from the same file.

Every key falls back **independently** if the file, the section, or just that one key is missing — this is a correction from the previous version of this doc, which implied an all-or-nothing fallback. In practice a `config.ini` with only a `[Workspace]` section works fine; the title bar keys simply fall back to their own defaults untouched:

* `[Workspace] Width` → defaults to `800`
* `[Workspace] Height` → defaults to `600`
* `[TitleBar] Location` → defaults to `0` (Top)
* `[TitleBar] Thickness` → defaults to `40`
* `[TitleBar] Color` → defaults to `0xFF313244`

If the whole file or directory is absent, every key above simply takes its default at once — same mechanism, just every call missing its target.


