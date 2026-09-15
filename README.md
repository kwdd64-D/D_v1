# D — Containerless Workspace Engine

A high-performance, architecture-ready, borderless user interface desktop engine written from scratch in x64 Windows Assembly (**FASM 1**). 

## 🧠 Design Philosophy
Unlike heavy, object-oriented modern graphics libraries, this system operates on a **stateless, containerless philosophy**. 
* **Zero UI Objects:** Every component and widget exists only as a collection of flat numbers `(x, y, w, h, color)` inside raw memory arrays.
* **Direct Pixel Mapping:** The CPU calculates index-arithmetic offsets globally and writes 32-bit hex colors (`$AARRGGBB`) straight into raw RAM.
* **Platform Independence:** The logic and drawing code are completely separated from the operating system hooks, making the rendering math 100% ready for future compilation onto cross-platform silicon (ARM64, RISC-V, etc.).

## 📁 Repository Map
* 📁 `assets/` — bin based images
* 📁 `config/` — Text-based layout scripts (`config.ini`) controlling user-space dimensions.
* 📁 `lib/` — Shared infrastructure binaries and stateless graphics kernel primitives.
* 📁 `src/` — Master timeline assembly drivers (source only).
* 📁 `build/` — Output binaries land here (`build.bat` creates it automatically; not committed to the repo).

## 🚀 Build Instructions
Requirements:
* Windows x64
* [FASM 1](https://flatassemblerorg.github.io/) installed anywhere on your machine.

Clone or copy this repo anywhere you like — no fixed path required. All project-local includes and the `config.ini`/`symbols.bin` lookups are resolved relative to the repo's own folder structure, and `config.ini` itself is located at *runtime* relative to wherever `workspace.exe` ends up, not a hardcoded drive.

The one machine-specific detail is telling the build where **your** FASM install lives, so its own headers (`WIN64W.INC`, `API\KERNEL32.INC`, etc.) can be found. That setting lives in **`build.bat`**, not in the source:

```cmd
build.bat
```

The first time you build, open `build.bat` and edit the single `set FASM_INCLUDE=C:\fasm\INCLUDE` line to match your FASM install, then just run the script. `workspace.asm` itself never needs editing. The script creates `build\` if it doesn't exist yet and drops `workspace.exe` there, keeping compiled output separate from source.

If you'd rather build by hand:
```cmd
set INCLUDE=C:\fasm\INCLUDE
cd D_v1
mkdir build
fasm src\workspace.asm build\workspace.exe
```
