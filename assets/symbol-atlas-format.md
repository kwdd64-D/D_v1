# Symbol Atlas File Format — Reference for Generating Icon Sheets

Companion format to `atlas-format.md` (the glyph atlas), for a **separate,
language-independent sheet of tintable UI icons** — window controls,
navigation arrows, etc. This is a draft spec: nothing here has been
verified against a working loader/blit yet, unlike the glyph format it's
modeled on. Treat it as the target to implement against, not (yet) a
description of shipped, tested behavior.

---

## 1. File-level layout

- **No header. No footer. No padding.** Same discipline as the glyph
  atlas — raw pixel bytes, icon after icon, back to back.
- **Icon count: 10**, for the current control set (see §4). Not a fixed
  constant the way 96 is for ASCII — this format has no implicit range,
  so the count is just however many named icons exist right now. Adding
  a new icon later means appending one more cell at the end of the file
  and one more name in the index table (§4) — never inserting or
  reordering, since every other icon's offset is computed from its index.
- **Total size: 2,560 bytes** at the current count (10 × 256 — see §2).
  Recompute as `icon_count × 256` whenever the count changes.

## 2. Per-icon layout

- Each icon is a **16×16 pixel cell** — deliberately smaller than the
  glyph atlas's 32×32, since these render at title-bar/control scale, not
  reading scale.
- Each pixel is **1 byte: coverage (alpha)**, not 3-byte RGB. There is no
  stored color — every icon is implicitly pure white, and the byte at
  each pixel says how much of that white shows through when the icon is
  tinted at draw time. `0` = fully transparent (background shows through
  untouched), `255` = fully opaque white, anything between is a partial /
  antialiased edge.
  - This is the *same technique* as the glyph atlas's gray antialiasing
    ramp, just reinterpreted: there, a mid-gray pixel blended visually
    between black and white; here, a mid-value byte blends the tint color
    into whatever's already in the framebuffer at that pixel.
- **Row size: 16 bytes** (16 pixels × 1 byte).
- **Icon size: 256 bytes** (16 rows × 16 bytes).
- Icon `N` starts at file offset `N × 256`, no gaps, no exceptions —
  same fixed-stride convention as the glyph atlas.

```
file offset 0          → icon 0 (ICON_MINIMIZE), row 0, pixel 0, coverage
file offset 1          → icon 0, row 0, pixel 1, coverage
...
file offset 16         → icon 0, row 1, pixel 0, coverage
...
file offset 256        → icon 1 (ICON_MAXIMIZE), row 0, pixel 0, coverage
...
file offset 9 * 256    → icon 9 (ICON_MAX_HEIGHT), row 0, pixel 0, coverage
file offset 2559       → last byte of the file
```

## 3. Color convention

- **No stored color, no background fill.** Unlike the glyph atlas, there
  is no white background pixel to paint — `0` coverage means "don't touch
  the framebuffer here," full stop. This format has no color-key or
  transparency *ambiguity* to worry about, because there's no color
  channel to begin with.
- **The blit is compositing, not overwrite.** `blit_glyph` in the glyph
  format copies every byte verbatim; a symbol blit must instead do, per
  pixel, something like `dest = dest*(1 - a/255) + tint*(a/255)` where
  `tint` is the current theme color (e.g. resolved from OKLCH, see the
  title bar work) and `a` is the stored coverage byte. This is a
  genuinely different routine from `blit_glyph` — not a flag or variant
  of it.
- **One tint per draw call, not per pixel.** The OKLCH → RGB resolution
  happens once per theme/draw (cheap), the blit just multiplies that
  single fixed color by each pixel's stored alpha (needs to be fast,
  since it runs per pixel).

## 4. Icon index table (no implicit ordering — must be explicit)

The glyph atlas gets its index for free (`ascii - 0x20`). Icons have no
natural ordering, so the index is just a plain enumeration, defined once
and shared by whatever generates the file and whatever blits from it:

| Index | Name              | Notes                                             |
|-------|-------------------|----------------------------------------------------|
| 0     | `ICON_MINIMIZE`   |                                                    |
| 1     | `ICON_MAXIMIZE`   |                                                    |
| 2     | `ICON_CLOSE`      |                                                    |
| 3     | `ICON_TITLEBAR`   | app/window icon, not a clickable control          |
| 4     | `ICON_LEFT`       | arrow                                             |
| 5     | `ICON_RIGHT`      | arrow                                             |
| 6     | `ICON_REFRESH`    |                                                    |
| 7     | `ICON_HAMBURGER`  | menu                                              |
| 8     | `ICON_MAX_WIDTH`  | resize handler affects width only — art-side note |
| 9     | `ICON_MAX_HEIGHT` | resize handler affects height only — art-side note|

`ICON_COUNT = 10`. This table should exist as one small `.inc` of
constants that both the atlas-generating tool and any blit call site
include, so nothing ever references an icon by a bare number. Adding
icon 10 later is: add a row here, add a 256-byte cell at file offset
`10 × 256`, bump `ICON_COUNT`. Nothing already shipped moves.

The width-only/height-only distinction between icons 8 and 9 is a
**behavioral** note for whatever resize-handling code eventually
dispatches on these icons (icon 8's click handler must only ever touch
width, icon 9's only height) — it has no bearing on the pixel format
itself, since they're just two ordinary, visually-distinct cells.

## 5. Filename / delivery

- Mirrors the glyph atlas convention: a CHAR16 (UTF-16LE) filename,
  null-terminated, at the root of the boot volume.
- Suggest a constant name analogous to `SC_ATLAS_FILENAME` —
  e.g. `SC_SYMBOL_FILENAME` in `d-context.inc` — with a core-level default
  (e.g. `symbols.bin`) if nothing overrides it. Unlike the glyph atlas,
  this sheet is **not** expected to vary per language cart, so most carts
  should simply not set this and fall through to the default.
- Whether the loader wraps this file in the same 16-byte in-memory header
  the glyph atlas gets (§5 of `atlas-format.md`) is an open question for
  whenever the actual loader gets written — worth deciding for code-path
  consistency, but it doesn't change anything about the file itself.

## 6. What is *not* part of this file

- No header, footer, or padding on disk — same rule as the glyph atlas.
- No stored color/background — see §3.
- No index table on disk — the name→index mapping (§4) lives in source
  code shared by the generator and the blit code, never in the file
  itself. The file is exactly `ICON_COUNT × 256` bytes of coverage data
  and nothing else.

## 7. Validation checklist

1. **File size is exactly `ICON_COUNT × 256` bytes** — 2,560 at the
   current count of 10.
2. **`file_size / 256` equals `ICON_COUNT`** with no remainder.
3. **Render it back as a grid of grayscale images** (treat each stored
   byte as luminance, not alpha, purely for the purpose of *looking* at
   it) and visually confirm each cell looks like its intended icon,
   right-side up, right-side round.
4. **Spot-check a non-trivial icon isn't blank or fully solid** — e.g.
   `ICON_CLOSE` should have a meaningful spread of non-zero, non-255
   values (the antialiased X), not 0 (blank) or a uniform 255 (a solid
   square, i.e. wrong crop or wrong channel extracted from the source
   art).
5. **Confirm index-table order matches file order.** Since there's no
   implicit range to fall back on, an off-by-one here silently swaps two
   icons' meanings rather than producing an out-of-range error — this is
   the analogous failure mode to the glyph atlas's "wrong stride," but it
   won't look obviously garbled, so check it explicitly by name.

## 8. Reference implementation (sketch — not yet built)

```python
CELL = 16

# Icons in explicit index order — must match the .inc constants exactly.
ICON_ORDER = [
    'ICON_MINIMIZE', 'ICON_MAXIMIZE', 'ICON_CLOSE', 'ICON_TITLEBAR',
    'ICON_LEFT', 'ICON_RIGHT', 'ICON_REFRESH', 'ICON_HAMBURGER',
    'ICON_MAX_WIDTH', 'ICON_MAX_HEIGHT',
]

out = bytearray()
for name in ICON_ORDER:
    img = load_source_art(name)               # any size/format
    mask = to_16x16_coverage(img)              # -> single-channel, 0..255
    out += mask.tobytes()                      # raw coverage, row-major
# len(out) must equal len(ICON_ORDER) * 256
```

The two easiest mistakes, by analogy with the glyph tool: (a) letting
`ICON_ORDER` drift out of sync with the `.inc` constants — there's no
arithmetic relationship to catch this for you the way `ascii - 0x20`
does; and (b) accidentally exporting a color image instead of collapsing
it to a single coverage channel, which would silently produce a file
3× too large and shift every icon after the first.

