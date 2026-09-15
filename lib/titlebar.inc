; File: D:\D_v1\lib\titlebar.inc
; Title bar module — everything about the draggable caption bar lives here:
; its config (loaded from the ini), how it's painted into the frame buffer,
; and how WM_NCHITTEST recognizes the mouse being over it.
;
; Public surface (used elsewhere):
;   init_titlebar                    - macro, call once at startup to load config
;   draw_titlebar buf, canv_w, canv_h - macro, paints the bar per configured edge
;   hittest_titlebar hit, miss        - macro, jumps to hit/miss for WM_NCHITTEST
;   layout_close_icon canv_w, canv_h  - macro, positions the close button for
;                                        whichever edge is currently configured
;   hittest_close_button x, y, hit, miss
;                                      - macro, jumps to hit/miss depending on
;                                        whether (x,y) falls on the close button
;
; External dependencies (must exist before these macros are invoked):
;   _ini_path                  (workspace.asm)  - path to the ini file
;   draw_rect_block            (draw2d.inc)     - used by draw_titlebar
;   canvas_w / canvas_h        (workspace.asm)  - used by hittest_titlebar
;   mouse_pt                   (win64_host.inc) - used by hittest_titlebar
;   ICON_CELL                  (icon_ids.inc)   - used by the close button layout
;
; Config keys read from [TitleBar] in the ini:
;   Location  - 0=Top 1=Bottom 2=Left 3=Right (default 0)
;   Thickness - bar thickness in pixels (default 40)
;   Color     - 0xAARRGGBB, hex accepted via the "0x" prefix (default 0xFF313244)

CLOSE_MARGIN = 12   ; distance from the bar's far edge to the close button

section '.data' data readable writeable
    _ini_tb_section   du 'TitleBar', 0
    _ini_tb_location  du 'Location', 0
    _ini_tb_thickness du 'Thickness', 0
    _ini_tb_color     du 'Color', 0

    align 4
    titlebar_location  dd 0            ; 0=Top 1=Bottom 2=Left 3=Right
    titlebar_thickness dd 40
    titlebar_color     dd 0xFF313244

    align 4
    close_btn_x        dd 0            ; top-left corner of the close icon,
    close_btn_y        dd 0            ; recomputed by layout_close_icon

; --- Load [TitleBar] Location / Thickness / Color from the ini file ---
macro init_titlebar {
    invoke GetPrivateProfileIntW, _ini_tb_section, _ini_tb_location, 0, _ini_path
    mov [titlebar_location], eax

    invoke GetPrivateProfileIntW, _ini_tb_section, _ini_tb_thickness, 40, _ini_path
    mov [titlebar_thickness], eax

    invoke GetPrivateProfileIntW, _ini_tb_section, _ini_tb_color, 0xFF313244, _ini_path
    mov [titlebar_color], eax
}

; --- Paint the bar along whichever edge is configured ---
; buf_ptr        = frame buffer pointer
; canv_w, canv_h = 64-bit registers already holding the canvas dimensions
;                  (as workspace.asm prepares before its draw_rect_block calls)
macro draw_titlebar buf_ptr, canv_w, canv_h {
    local .loc_bottom, .loc_left, .loc_right, .loc_top, .done

    mov edx, [titlebar_color]

    mov eax, [titlebar_location]
    cmp eax, 1
    je .loc_bottom
    cmp eax, 2
    je .loc_left
    cmp eax, 3
    je .loc_right
    ; falls through to .loc_top for 0 and any unrecognized value

.loc_top:
    movsxd r13, dword [titlebar_thickness]
    draw_rect_block buf_ptr, canv_w, 0, 0, canv_w, r13, edx
    jmp .done

.loc_bottom:
    movsxd r13, dword [titlebar_thickness]
    mov r9, canv_h
    sub r9, r13
    draw_rect_block buf_ptr, canv_w, 0, r9, canv_w, r13, edx
    jmp .done

.loc_left:
    movsxd r13, dword [titlebar_thickness]
    draw_rect_block buf_ptr, canv_w, 0, 0, r13, canv_h, edx
    jmp .done

.loc_right:
    movsxd r13, dword [titlebar_thickness]
    mov r9, canv_w
    sub r9, r13
    draw_rect_block buf_ptr, canv_w, r9, 0, r13, canv_h, edx

.done:
}

; --- WM_NCHITTEST support ---
; Jumps to hit_label if mouse_pt currently falls inside the configured bar,
; otherwise jumps to miss_label. Reads canvas_w/canvas_h/mouse_pt directly,
; so call this after ScreenToClient has filled in mouse_pt.
macro hittest_titlebar hit_label, miss_label {
    local .check_bottom, .check_left, .check_right, .check_top

    mov eax, [titlebar_location]
    cmp eax, 1
    je .check_bottom
    cmp eax, 2
    je .check_left
    cmp eax, 3
    je .check_right

.check_top:
    movsxd rax, dword [mouse_pt.y]
    movsxd rcx, dword [titlebar_thickness]
    cmp rax, 0
    jl miss_label
    cmp rax, rcx
    jge miss_label
    jmp hit_label

.check_bottom:
    movsxd rax, dword [mouse_pt.y]
    movsxd rcx, dword [canvas_h]
    movsxd rdx, dword [titlebar_thickness]
    sub rcx, rdx
    cmp rax, rcx
    jl miss_label
    movsxd rcx, dword [canvas_h]
    cmp rax, rcx
    jge miss_label
    jmp hit_label

.check_left:
    movsxd rax, dword [mouse_pt.x]
    movsxd rcx, dword [titlebar_thickness]
    cmp rax, 0
    jl miss_label
    cmp rax, rcx
    jge miss_label
    jmp hit_label

.check_right:
    movsxd rax, dword [mouse_pt.x]
    movsxd rcx, dword [canvas_w]
    movsxd rdx, dword [titlebar_thickness]
    sub rcx, rdx
    cmp rax, rcx
    jl miss_label
    movsxd rcx, dword [canvas_w]
    cmp rax, rcx
    jge miss_label
    jmp hit_label
}

; --- Position the close button for whichever edge is configured ---
; Centered across the bar's thickness, offset CLOSE_MARGIN pixels in from
; the bar's far end (right end for Top/Bottom, top end for Left/Right —
; matching where a close button conventionally sits). Call after
; init_titlebar and after canv_w/canv_h are loaded into registers, and
; again any time the canvas size or title bar config changes.
macro layout_close_icon canv_w, canv_h {
    local .lt, .lb, .ll, .lr, .ldone

    mov eax, [titlebar_location]
    cmp eax, 1
    je .lb
    cmp eax, 2
    je .ll
    cmp eax, 3
    je .lr

.lt:
    mov rax, canv_w
    sub rax, CLOSE_MARGIN + ICON_CELL
    mov [close_btn_x], eax

    movsxd rbx, dword [titlebar_thickness]
    sub rbx, ICON_CELL
    sar rbx, 1
    mov [close_btn_y], ebx
    jmp .ldone

.lb:
    mov rax, canv_w
    sub rax, CLOSE_MARGIN + ICON_CELL
    mov [close_btn_x], eax

    movsxd r9, dword [titlebar_thickness]
    mov rax, canv_h
    sub rax, r9                 ; top edge of the bar
    mov rbx, r9
    sub rbx, ICON_CELL
    sar rbx, 1                  ; centered offset within the bar's thickness
    add rax, rbx
    mov [close_btn_y], eax
    jmp .ldone

.ll:
    movsxd rax, dword [titlebar_thickness]
    sub rax, ICON_CELL
    sar rax, 1
    mov [close_btn_x], eax
    mov dword [close_btn_y], CLOSE_MARGIN
    jmp .ldone

.lr:
    movsxd r9, dword [titlebar_thickness]
    mov rax, canv_w
    sub rax, r9                 ; left edge of the bar
    mov rbx, r9
    sub rbx, ICON_CELL
    sar rbx, 1
    add rax, rbx
    mov [close_btn_x], eax
    mov dword [close_btn_y], CLOSE_MARGIN

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
