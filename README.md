# D — Containerless Workspace Engine

A high-performance, architecture-ready, borderless user interface desktop engine written from scratch in x64 Windows Assembly (**FASM 1**). 

## 🧠 Design Philosophy
Unlike heavy, object-oriented modern graphics libraries, this system operates on a **stateless, containerless philosophy**. 
* **Zero UI Objects:** Every component and widget exists only as a collection of flat numbers `(x, y, w, h, color)` inside raw memory arrays.
* **Direct Pixel Mapping:** The CPU calculates index-arithmetic offsets globally and writes 32-bit hex colors (`$AARRGGBB`) straight into raw RAM.
* **Platform Independence:** The logic and drawing code are completely separated from the operating system hooks, making the rendering math 100% ready for future compilation onto cross-platform silicon (ARM64, RISC-V, etc.).

## 📁 Repository Map
* 📁 `config/` — Text-based layout scripts (`config.ini`) controlling user-space dimensions.
* 📁 `lib/` — Shared infrastructure binaries and stateless graphics kernel primitives.
* 📁 `src/` — Master timeline assembly drivers and output binaries.

## 🚀 Build Instructions
To build the project natively using the classic Flat Assembler compiler:
```cmd
C:\fasm>fasm D:\D_v1\src\workspace.asm D:\D_v1\src\workspace.exe
```
