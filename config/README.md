# 📁 Configuration Layer (`D:\D_v1\config\`)

This directory houses the human-readable text configuration scripts that determine your workspace size and bar parameters at initialization without requiring recompilation.

## 📄 `config.ini` Specifications
The main settings script uses standard Windows `.ini` structuring parsed natively by the OS desktop manager subsystem:

```ini
[Workspace]
Width=1024        ; Dynamic width of the frame buffer canvas in pixels
Height=768        ; Dynamic height of the frame buffer canvas in pixels

[Bar]
Location=0        ; Which edge hosts the bar: 0=Top 1=Bottom 2=Left 3=Right
Thickness=40      ; Bar thickness in pixels, measured across the short axis
Color=0xFF313244  ; 0xAARRGGBB — hex accepted via the "0x" prefix
```

## 🛠️ Execution Mechanics
At boot time, the master assembly driver queries this directory to extract text values into live memory variables.

- If this directory or file is ever entirely absent, the rendering engine safely defaults back to an `800x600` target container resolution.
- If `[Bar]` is absent, or any of its three keys are missing, each one falls back independently: `Location=0` (Top), `Thickness=40`, `Color=0xFF313244`.
- **Confirmed working:** `Location` has been tested at all four values — the bar, its drag region, and the close button all reposition correctly for Top, Bottom, Left, and Right.
