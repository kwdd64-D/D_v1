; File: D:\D_v1\lib\bar.inc
; Bar module — everything about the draggable chrome bar lives here: its
; configuration, how it's painted into the frame buffer, and how
; WM_NCHITTEST recognizes the mouse being over it. Called "bar," not
; "titlebar" — it doesn't have to be at the top, and it doesn't render a
; title, so a name implying either would be lying about what this is.
;
; Public surface (used elsewhere):
;   init_bar                     - macro, call once at startup to load config
;   draw_bar buf, canv_w, canv_h - macro, paints the bar per configured edge
;   hittest_bar hit, miss        - macro, jumps to hit/miss for WM_NCHITTEST
;   layout_close_icon canv_w, canv_h  - macro, positions the close button for
;                                        whichever edge is currently configured
;   layout_icon_near_close slot, out_x, out_y
;                                      - macro, positions an icon stacked next
;                                        to the close button (slot 1, 2, ...
;                                        step progressively further from the
;                                        corner close sits in)
;   layout_app_icon canv_w, canv_h     - macro, positions the app icon at the
;                                        end of the bar opposite the close
;                                        button cluster
;   layout_center_icons canv_w, canv_h - macro, positions the max-width/
;                                        max-height pair centered on the bar
;   hittest_close_button x, y, hit, miss
;                                      - macro, jumps to hit/miss depending on
;                                        whether (x,y) falls on the close button
;
; None of layout_icon_near_close / layout_app_icon / layout_center_icons
; have a matching hit-test yet — they're draw-only until something needs
; to click them.
;
; External dependencies (must exist before these macros are invoked):
;   _ini_path                  (workspace.asm)  - path to the ini file
;   draw_rect_block            (draw2d.inc)     - used by draw_bar
;   canvas_w / canvas_h        (workspace.asm)  - used by hittest_bar and
;                                                  layout_icon_near_close
;   mouse_pt                   (win64_host.inc) - used by hittest_bar
;   ICON_CELL                  (icon_ids.inc)   - used by every icon layout macro
;
; Config keys read from [Bar] in the ini:
;   Location  - 0=Top 1=Bottom 2=Left 3=Right (default 0)
;   Thickness - bar thickness in pixels (default 40)
;   Color     - 0xAARRGGBB, hex accepted via the "0x" prefix (default 0xFF313244)

ICON_MARGIN  = 12   ; distance from the bar's near/far edge to its end icons
ICON_SPACING = 4    ; gap between two icons stacked next to each other

section '.data' data readable writeable
    _ini_bar_section   du 'Bar', 0
    _ini_bar_location  du 'Location', 0
    _ini_bar_thickness du 'Thickness', 0
    _ini_bar_color     du 'Color', 0

    align 4
    bar_location  dd 0            ; 0=Top 1=Bottom 2=Left 3=Right
    bar_thickness dd 40
    bar_color     dd 0xFF313244

    align 4
    close_btn_x   dd 0            ; close button — far corner, slot 0
    close_btn_y   dd 0
    max_btn_x     dd 0            ; maximize — one slot in from close
    max_btn_y     dd 0
    min_btn_x     dd 0            ; minimize — two slots in from close
    min_btn_y     dd 0
    app_icon_x    dd 0            ; app icon — opposite end from close
    app_icon_y    dd 0
    max_width_x   dd 0            ; centered resize-width icon
    max_width_y   dd 0
    max_height_x  dd 0            ; centered resize-height icon
    max_height_y  dd 0

; --- Load [Bar] Location / Thickness / Color from the ini file ---
macro init_bar {
    invoke GetPrivateProfileIntW, _ini_bar_section, _ini_bar_location, 0, _ini_path
    mov [bar_location], eax

    invoke GetPrivateProfileIntW, _ini_bar_section, _ini_bar_thickness, 40, _ini_path
    mov [bar_thickness], eax

    invoke GetPrivateProfileIntW, _ini_bar_section, _ini_bar_color, 0xFF313244, _ini_path
    mov [bar_color], eax
}

; --- Paint the bar along whichever edge is configured ---
; buf_ptr        = frame buffer pointer
; canv_w, canv_h = 64-bit registers already holding the canvas dimensions
;                  (as workspace.asm prepares before its draw_rect_block calls)
macro draw_bar buf_ptr, canv_w, canv_h {
    local .loc_bottom, .loc_left, .loc_right, .loc_top, .done

    mov edx, [bar_color]

    mov eax, [bar_location]
    cmp eax, 1
    je .loc_bottom
    cmp eax, 2
    je .loc_left
    cmp eax, 3
    je .loc_right
    ; falls through to .loc_top for 0 and any unrecognized value

.loc_top:
    movsxd r13, dword [bar_thickness]
    draw_rect_block buf_ptr, canv_w, 0, 0, canv_w, r13, edx
    jmp .done

.loc_bottom:
    movsxd r13, dword [bar_thickness]
    mov r9, canv_h
    sub r9, r13
    draw_rect_block buf_ptr, canv_w, 0, r9, canv_w, r13, edx
    jmp .done

.loc_left:
    movsxd r13, dword [bar_thickness]
    draw_rect_block buf_ptr, canv_w, 0, 0, r13, canv_h, edx
    jmp .done

.loc_right:
    movsxd r13, dword [bar_thickness]
    mov r9, canv_w
    sub r9, r13
    draw_rect_block buf_ptr, canv_w, r9, 0, r13, canv_h, edx

.done:
}

; --- WM_NCHITTEST support ---
; Jumps to hit_label if mouse_pt currently falls inside the configured bar,
; otherwise jumps to miss_label. Reads canvas_w/canvas_h/mouse_pt directly,
; so call this after ScreenToClient has filled in mouse_pt.
macro hittest_bar hit_label, miss_label {
    local .check_bottom, .check_left, .check_right, .check_top

    mov eax, [bar_location]
    cmp eax, 1
    je .check_bottom
    cmp eax, 2
    je .check_left
    cmp eax, 3
    je .check_right

.check_top:
    movsxd rax, dword [mouse_pt.y]
    movsxd rcx, dword [bar_thickness]
    cmp rax, 0
    jl miss_label
    cmp rax, rcx
    jge miss_label
    jmp hit_label

.check_bottom:
    movsxd rax, dword [mouse_pt.y]
    movsxd rcx, dword [canvas_h]
    movsxd rdx, dword [bar_thickness]
    sub rcx, rdx
    cmp rax, rcx
    jl miss_label
    movsxd rcx, dword [canvas_h]
    cmp rax, rcx
    jge miss_label
    jmp hit_label

.check_left:
    movsxd rax, dword [mouse_pt.x]
    movsxd rcx, dword [bar_thickness]
    cmp rax, 0
    jl miss_label
    cmp rax, rcx
    jge miss_label
    jmp hit_label

.check_right:
    movsxd rax, dword [mouse_pt.x]
    movsxd rcx, dword [canvas_w]
    movsxd rdx, dword [bar_thickness]
    sub rcx, rdx
    cmp rax, rcx
    jl miss_label
    movsxd rcx, dword [canvas_w]
    cmp rax, rcx
    jge miss_label
    jmp hit_label
}

; --- Position the close button for whichever edge is configured ---
; Centered across the bar's thickness, offset ICON_MARGIN pixels in from
; the bar's far end (right end for Top/Bottom, top end for Left/Right —
; matching where a close button conventionally sits). Call after init_bar
; and after canv_w/canv_h are loaded into registers, and again any time
; the canvas size or bar config changes.
macro layout_close_icon canv_w, canv_h {
    local .lt, .lb, .ll, .lr, .ldone

    mov eax, [bar_location]
    cmp eax, 1
    je .lb
    cmp eax, 2
    je .ll
    cmp eax, 3
    je .lr

.lt:
    mov rax, canv_w
    sub rax, ICON_MARGIN + ICON_CELL
    mov [close_btn_x], eax

    movsxd rbx, dword [bar_thickness]
    sub rbx, ICON_CELL
    sar rbx, 1
    mov [close_btn_y], ebx
    jmp .ldone

.lb:
    mov rax, canv_w
    sub rax, ICON_MARGIN + ICON_CELL
    mov [close_btn_x], eax

    movsxd r9, dword [bar_thickness]
    mov rax, canv_h
    sub rax, r9                 ; top edge of the bar
    mov rbx, r9
    sub rbx, ICON_CELL
    sar rbx, 1                  ; centered offset within the bar's thickness
    add rax, rbx
    mov [close_btn_y], eax
    jmp .ldone

.ll:
    movsxd rax, dword [bar_thickness]
    sub rax, ICON_CELL
    sar rax, 1
    mov [close_btn_x], eax
    mov dword [close_btn_y], ICON_MARGIN
    jmp .ldone

.lr:
    movsxd r9, dword [bar_thickness]
    mov rax, canv_w
    sub rax, r9                 ; left edge of the bar
    mov rbx, r9
    sub rbx, ICON_CELL
    sar rbx, 1
    add rax, rbx
    mov [close_btn_x], eax
    mov dword [close_btn_y], ICON_MARGIN

.ldone:
}

; --- Hit-test the close button ---
; Jumps to hit_label if (x_reg, y_reg) — a client-coordinate point, in
; whatever 64-bit registers the caller has it in — falls on the close
; button; otherwise falls to miss_label. Works the same whether the point
; came from WM_LBUTTONDOWN's lParam directly or from mouse_pt after
; ScreenToClient, so the same macro serves both the click handler and the
; WM_NCHITTEST exclusion.
macro hittest_close_button x_reg, y_reg, hit_label, miss_label {
    movsxd rax, dword [close_btn_x]
    cmp x_reg, rax
    jl miss_label
    add rax, ICON_CELL
    cmp x_reg, rax
    jge miss_label

    movsxd rax, dword [close_btn_y]
    cmp y_reg, rax
    jl miss_label
    add rax, ICON_CELL
    cmp y_reg, rax
    jge miss_label

    jmp hit_label
}

; --- Position an icon stacked inward from the close button ---
; slot is a compile-time number: 1 sits one icon-width-plus-gap further
; in from where close itself sits, 2 sits two further in, and so on
; (don't pass 0 — that's close's own spot, already owned by
; layout_close_icon). Unlike the other layout macros here, this one
; reads canvas_w/canvas_h straight from memory instead of taking them as
; register parameters, since it's meant to be called for each stacked
; icon in turn without the caller having to keep re-passing them.
; out_x/out_y are wherever the caller wants the result written — e.g.
; max_btn_x/max_btn_y for slot 1, min_btn_x/min_btn_y for slot 2.
macro layout_icon_near_close slot, out_x, out_y {
    local .use_h, .have_l, .t, .b, .l, .r, .done

    movsxd rax, dword [canvas_w]
    cmp dword [bar_location], 2
    je .use_h
    cmp dword [bar_location], 3
    je .use_h
    jmp .have_l
.use_h:
    movsxd rax, dword [canvas_h]
.have_l:
    sub rax, ICON_MARGIN + (slot+1)*ICON_CELL + slot*ICON_SPACING
    ; rax = along, measured from the far end inward

    movsxd rcx, dword [bar_thickness]
    sub rcx, ICON_CELL
    sar rcx, 1
    ; rcx = cross, centered across the bar's thickness

    mov edx, [bar_location]
    cmp edx, 1
    je .b
    cmp edx, 2
    je .l
    cmp edx, 3
    je .r

.t:
    mov [out_x], eax
    mov [out_y], ecx
    jmp .done

.b:
    mov [out_x], eax
    movsxd rdx, dword [canvas_h]
    movsxd rbx, dword [bar_thickness]
    sub rdx, rbx
    add rdx, rcx
    mov [out_y], edx
    jmp .done

.l:
    mov [out_x], ecx
    mov [out_y], eax
    jmp .done

.r:
    movsxd rdx, dword [canvas_w]
    movsxd rbx, dword [bar_thickness]
    sub rdx, rbx
    add rdx, rcx
    mov [out_x], edx
    mov [out_y], eax

.done:
}

; --- Position the app icon at the end of the bar opposite the close
; button cluster ---
; Same shape as layout_close_icon, mirrored to the near end instead of
; the far end (no need to subtract from the axis length — ICON_MARGIN
; from the start is the whole answer). Call after init_bar and after
; canv_w/canv_h are loaded into registers.
macro layout_app_icon canv_w, canv_h {
    local .lt, .lb, .ll, .lr, .ldone

    mov eax, [bar_location]
    cmp eax, 1
    je .lb
    cmp eax, 2
    je .ll
    cmp eax, 3
    je .lr

.lt:
    mov dword [app_icon_x], ICON_MARGIN
    movsxd rbx, dword [bar_thickness]
    sub rbx, ICON_CELL
    sar rbx, 1
    mov [app_icon_y], ebx
    jmp .ldone

.lb:
    mov dword [app_icon_x], ICON_MARGIN
    movsxd r9, dword [bar_thickness]
    mov rax, canv_h
    sub rax, r9
    mov rbx, r9
    sub rbx, ICON_CELL
    sar rbx, 1
    add rax, rbx
    mov [app_icon_y], eax
    jmp .ldone

.ll:
    movsxd rax, dword [bar_thickness]
    sub rax, ICON_CELL
    sar rax, 1
    mov [app_icon_x], eax
    mov dword [app_icon_y], ICON_MARGIN
    jmp .ldone

.lr:
    movsxd r9, dword [bar_thickness]
    mov rax, canv_w
    sub rax, r9
    mov rbx, r9
    sub rbx, ICON_CELL
    sar rbx, 1
    add rax, rbx
    mov [app_icon_x], eax
    mov dword [app_icon_y], ICON_MARGIN

.ldone:
}

; --- Position the max-width/max-height pair centered on the bar ---
; The two sit side by side (or stacked, on a vertical bar), centered as
; a unit along the bar's long axis, sharing the same cross-axis offset.
; Call after init_bar and after canv_w/canv_h are loaded into registers.
macro layout_center_icons canv_w, canv_h {
    local .lt, .lb, .ll, .lr, .ldone

    mov eax, [bar_location]
    cmp eax, 1
    je .lb
    cmp eax, 2
    je .ll
    cmp eax, 3
    je .lr

.lt:
    mov rax, canv_w
    sub rax, 2*ICON_CELL + ICON_SPACING
    sar rax, 1
    mov [max_width_x], eax
    add eax, ICON_CELL + ICON_SPACING
    mov [max_height_x], eax

    movsxd rbx, dword [bar_thickness]
    sub rbx, ICON_CELL
    sar rbx, 1
    mov [max_width_y], ebx
    mov [max_height_y], ebx
    jmp .ldone

.lb:
    mov rax, canv_w
    sub rax, 2*ICON_CELL + ICON_SPACING
    sar rax, 1
    mov [max_width_x], eax
    add eax, ICON_CELL + ICON_SPACING
    mov [max_height_x], eax

    movsxd r9, dword [bar_thickness]
    mov rbx, canv_h
    sub rbx, r9
    mov rcx, r9
    sub rcx, ICON_CELL
    sar rcx, 1
    add rbx, rcx
    mov [max_width_y], ebx
    mov [max_height_y], ebx
    jmp .ldone

.ll:
    mov rax, canv_h
    sub rax, 2*ICON_CELL + ICON_SPACING
    sar rax, 1
    mov [max_width_y], eax
    add eax, ICON_CELL + ICON_SPACING
    mov [max_height_y], eax

    movsxd rbx, dword [bar_thickness]
    sub rbx, ICON_CELL
    sar rbx, 1
    mov [max_width_x], ebx
    mov [max_height_x], ebx
    jmp .ldone

.lr:
    mov rax, canv_h
    sub rax, 2*ICON_CELL + ICON_SPACING
    sar rax, 1
    mov [max_width_y], eax
    add eax, ICON_CELL + ICON_SPACING
    mov [max_height_y], eax

    movsxd r9, dword [bar_thickness]
    mov rbx, canv_w
    sub rbx, r9
    mov rcx, r9
    sub rcx, ICON_CELL
    sar rcx, 1
    add rbx, rcx
    mov [max_width_x], ebx
    mov [max_height_x], ebx

.ldone:
}
