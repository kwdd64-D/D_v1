; File: D:\D_v1\lib\win64_host.inc
; Windows x64 host presentation and event wrapper platform layer

DIB_RGB_COLORS = 0
SW_SHOWNORMAL  = 1
CS_VREDRAW     = 1
CS_HREDRAW     = 2
WM_NCHITTEST   = 0x0084  
WM_LBUTTONDOWN = 0x0201
HTCAPTION      = 2       

section '.data' data readable writeable
    wc           WNDCLASSEX sizeof.WNDCLASSEX, CS_VREDRAW or CS_HREDRAW, WindowProc, 0, 0, 0, 0, 0, 0, 0, _class, 0
    msg          MSG
    ps           PAINTSTRUCT
    pixel_buffer dq 0
    hwnd         dq 0
    hinstance    dq 0

    align 8
    mouse_pt:
        .x dd 0
        .y dd 0

    align 8
    bmi:
        dd 40
        dd 0          ; Filled dynamically by workspace initialization step
        dd 0          ; Filled dynamically by workspace initialization step
        dw 1
        dw 32
        dd 0, 0, 0, 0, 0, 0, 0

section '.text' code readable executable
proc WindowProc uses rbx rsi rdi r12, hwnd, uMsg, wParam, lParam
    mov [hwnd], rcx
    mov [uMsg], rdx
    mov [wParam], r8
    mov [lParam], r9

    cmp edx, WM_PAINT
    je .on_paint
    cmp edx, WM_NCHITTEST       
    je .on_hit_test
    cmp edx, WM_LBUTTONDOWN
    je .on_lbuttondown
    cmp edx, WM_DESTROY
    je .on_destroy

    invoke DefWindowProc, [hwnd], [uMsg], [wParam], [lParam]
    jmp .done

.on_hit_test:
    mov rax, [lParam]
    movsx rbx, ax               
    sar rax, 16
    movsx rcx, ax               
    mov [mouse_pt.x], ebx
    mov [mouse_pt.y], ecx

    invoke ScreenToClient, [hwnd], mouse_pt

    ; The close button always wins over the general bar drag region,
    ; regardless of which edge the bar is on — a hit there falls straight
    ; through to DefWindowProc (ordinary HTCLIENT), so the click reaches
    ; WindowProc as a normal WM_LBUTTONDOWN instead of starting a drag.
    movsxd rbx, dword [mouse_pt.x]
    movsxd rcx, dword [mouse_pt.y]
    hittest_close_button rbx, rcx, .not_in_bar, .check_bar
.check_bar:
    hittest_titlebar .in_bar, .not_in_bar

.in_bar:
    mov rax, HTCAPTION
    jmp .done
.not_in_bar:
    invoke DefWindowProc, [hwnd], [uMsg], [wParam], [lParam]
    jmp .done

.on_lbuttondown:
    ; WM_LBUTTONDOWN's lParam is already client-coordinate, unlike
    ; WM_NCHITTEST — no ScreenToClient needed here.
    mov rax, [lParam]
    movsx rbx, ax
    sar rax, 16
    movsx rcx, ax
    hittest_close_button rbx, rcx, .do_close, .lbtn_done

.do_close:
    invoke DestroyWindow, [hwnd]
.lbtn_done:
    xor rax, rax
    jmp .done

.on_paint:
    invoke BeginPaint, [hwnd], ps
    mov r12, rax
    invoke StretchDIBits, r12, 0, 0, [canvas_w], [canvas_h], 0, 0, [canvas_w], [canvas_h], [pixel_buffer], bmi, DIB_RGB_COLORS, SRCCOPY
    invoke EndPaint, [hwnd], ps
    xor rax, rax
    jmp .done

.on_destroy:
    invoke PostQuitMessage, 0
    xor rax, rax
.done:
    ret
endp
