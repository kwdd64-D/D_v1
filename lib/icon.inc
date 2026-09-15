; File: lib\icon.inc
; Generic tintable icon rendering. Owns the embedded symbol atlas bytes
; and the mask+tint compositing blit — nothing else. Doesn't know which
; icon means what (that's icon_ids.inc) or where on screen it belongs
; (that's the caller's job — e.g. titlebar.inc for the window-chrome
; buttons).
;
; Depends on:
;   icon_ids.inc (ICON_CELL / ICON_CELL_BYTES)
;
; Public surface:
;   draw_icon_masked buf, canv_w, x, y, icon_index, tint

section '.data' data readable writeable
    ; Embedded straight from the generated atlas (see symbol-atlas-format.md).
    ; This is a build-time embed, not a runtime load — no file I/O, no
    ; allocation, nothing to fail at startup. Swap for a real loader later
    ; if icons ever need to be hot-swappable independent of a rebuild.
    align 4
    symbols_data:
        file '..\assets\symbols.bin'

; Paints one 16x16 coverage-masked icon into the frame buffer, tinted
; with a flat color. Per pixel: coverage 0 leaves the destination
; untouched, 255 fully replaces it with tint, values between blend
; proportionally — dest = dest*(1-a) + tint*a, done per channel.
;
; buf_ptr    = frame buffer pointer
; canv_w     = 64-bit register/value holding canvas width in pixels
; x, y       = destination top-left corner, in pixels
; icon_index = one of the ICON_* constants from icon_ids.inc
; tint       = 0x00RRGGBB color (top byte ignored) to composite in
;
; Registers r8/r9/r10/r13/r14/r15 + rax/rbx/rcx/rdx/rsi/rdi are used as
; scratch. r11 and r12 are deliberately left untouched, since callers
; (workspace.asm) keep canvas width/height cached there across multiple
; draw calls in a row.
;
; x and y must be 64-bit register or immediate values, not raw memory
; operands (mixing a 64-bit mov with a dword-sized variable is a size
; mismatch FASM will reject) — sign-extend with movsxd first, same as
; canvas_w/canvas_h get promoted into r11/r12 before any of these macros
; are called. Same convention as draw_rect_block: x and y are read once,
; right at the top, before this macro's own scratch registers get reused
; — so it's safe to pass e.g. r9/r10 straight through as x/y even though
; the macro later clobbers r9/r10 internally for its own loop counters.
macro draw_icon_masked buf_ptr, canv_w, x, y, icon_index, tint {
    local .row, .col

    ; mask pointer = symbols_data + icon_index * ICON_CELL_BYTES
    mov rsi, icon_index
    imul rsi, rsi, ICON_CELL_BYTES
    add rsi, symbols_data

    ; dest pixel pointer = buf + ((y * canv_w) + x) * 4
    mov rax, y
    imul rax, canv_w
    add rax, x
    shl rax, 2
    mov rdi, [buf_ptr]
    add rdi, rax

    ; row stride padding = (canv_w - ICON_CELL) * 4
    mov r10, canv_w
    sub r10, ICON_CELL
    shl r10, 2

    ; unpack tint into its three channels (low byte = B, then G, then R)
    mov eax, tint
    movzx r13d, al
    mov ebx, eax
    shr ebx, 8
    movzx r14d, bl
    mov ebx, eax
    shr ebx, 16
    movzx r15d, bl

    mov r8, ICON_CELL
.row:
    mov r9, ICON_CELL
.col:
    movzx ecx, byte [rsi]        ; coverage a, 0..255
    inc rsi

    ; Each channel: x = dest*(255-a) + tint*a ; result = fast-divide(x, 255)
    ; using the exact identity floor(x/255) == (x + 1 + (x>>8)) >> 8
    ; for x in [0, 65025], which this x always falls within.

    ; --- Blue ---
    movzx ebx, byte [rdi]
    mov edx, 255
    sub edx, ecx
    imul edx, ebx
    mov eax, r13d
    imul eax, ecx
    add eax, edx
    mov edx, eax
    shr edx, 8
    add eax, edx
    add eax, 1
    shr eax, 8
    mov [rdi], al

    ; --- Green ---
    movzx ebx, byte [rdi+1]
    mov edx, 255
    sub edx, ecx
    imul edx, ebx
    mov eax, r14d
    imul eax, ecx
    add eax, edx
    mov edx, eax
    shr edx, 8
    add eax, edx
    add eax, 1
    shr eax, 8
    mov [rdi+1], al

    ; --- Red ---
    movzx ebx, byte [rdi+2]
    mov edx, 255
    sub edx, ecx
    imul edx, ebx
    mov eax, r15d
    imul eax, ecx
    add eax, edx
    mov edx, eax
    shr edx, 8
    add eax, edx
    add eax, 1
    shr eax, 8
    mov [rdi+2], al

    add rdi, 4
    dec r9
    jnz .col

    add rdi, r10
    dec r8
    jnz .row
}
