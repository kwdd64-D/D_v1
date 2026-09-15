; File: D:\D_v1\lib\draw2d.inc
; Reusable, hardware-agnostic 2D frame buffer components

; Stateless Portable Rect Block Burner
; Expects raw 64-bit values or registers for coordinates and sizing
macro draw_rect_block buf_ptr, canv_w, rect_x, rect_y, rect_w, rect_h, color {
    local .loop_y, .loop_x
    
    ; Move tracking bounds to temporary registers
    mov r11, canv_w
    mov r14, rect_w
    mov r15, rect_h
    
    ; Compute absolute byte offset index: ((Y * CanvasWidth) + X) * 4
    mov rax, rect_y
    imul rax, r11             
    add rax, rect_x
    shl rax, 2                ; Multiply by 4 (bytes per pixel)
    
    mov rdi, [buf_ptr]
    add rdi, rax             
    
    ; Calculate row scan padding jump: (CanvasWidth - RectWidth) * 4
    mov r10, r11
    sub r10, r14              
    shl r10, 2               
    
    mov r8, r15               
.loop_y:
    mov rcx, r14              
.loop_x:
    mov dword [rdi], color   
    add rdi, 4
    sub rcx, 1
    jnz .loop_x
    
    add rdi, r10             
    sub r8, 1
    jnz .loop_y
}
