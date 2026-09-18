; File: D:\D_v1\lib\icon_ids.inc
; Name -> index table for symbols.bin (see symbol-atlas-format.md, section 4).
; The glyph atlas gets its index for free from ASCII; this one doesn't, so
; every place that draws or hit-tests an icon must go through these names,
; never a bare numeric index. Adding a new icon later: append a new
; constant here (next number up), append its 256-byte cell to the end of
; symbols.bin, bump ICON_COUNT. Never renumber an existing one.

ICON_MINIMIZE   = 0
ICON_MAXIMIZE   = 1
ICON_CLOSE      = 2
ICON_APP        = 3
ICON_LEFT       = 4
ICON_RIGHT      = 5
ICON_REFRESH    = 6
ICON_HAMBURGER  = 7
ICON_MAX_WIDTH  = 8
ICON_MAX_HEIGHT = 9

ICON_COUNT      = 10

ICON_CELL       = 16                       ; pixels, both dimensions
ICON_CELL_BYTES = ICON_CELL * ICON_CELL    ; 256 coverage bytes per icon
