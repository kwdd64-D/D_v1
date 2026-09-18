format PE64 GUI 5.0
entry start

include 'C:\fasm\INCLUDE\WIN64W.INC'

section '.data' data readable writeable
    _class        TCHAR 'NakedCanvasClass', 0
    _title        TCHAR 'Workspace', 0
    _error        TCHAR 'Window startup failed.', 0
    
    ; --- TARGET PATH POINTED TO YOUR CONFIG DIRECTORY ---
    _ini_path     du 'D:\D_v1\config\config.ini', 0
    _ini_section  du 'Workspace', 0
    _ini_width    du 'Width', 0
    _ini_height   du 'Height', 0

    ; --- DATA FIELD LAYOUT VARIABLE ALLOCATIONS ---
    align 4
    canvas_w    dd 0
    canvas_h    dd 0
    pitch_row   dq 0
    pitch_total dq 0

; Include your central decoupled libraries.
; Order matters here: icon_ids.inc/icon.inc must come before bar.inc
; (its close-button macros use ICON_CELL), and bar.inc must come before
; win64_host.inc (its WM_NCHITTEST/WM_LBUTTONDOWN handlers invoke bar.inc's
; macros, which need to already be defined).
include 'D:\D_v1\lib\icon_ids.inc'
include 'D:\D_v1\lib\icon.inc'
include 'D:\D_v1\lib\bar.inc'
include 'D:\D_v1\lib\win64_host.inc'
include 'D:\D_v1\lib\draw2d.inc'

section '.text' code readable executable
start:
    sub rsp, 40 ; Maintain rigid 16-byte shadow stack alignment boundaries

    ; 1. SUCK THE LIVE INI VARIABLES FROM DISK NATIVELY INTO REGISTERS
    invoke GetPrivateProfileIntW, _ini_section, _ini_width, 800, _ini_path
    mov [canvas_w], eax

    invoke GetPrivateProfileIntW, _ini_section, _ini_height, 600, _ini_path
    mov [canvas_h], eax

    ; Load [Bar] Location/Thickness/Color from the same ini file
    init_bar

    ; 2. RE-EVALUATE PITCH AND MEMORY BLOCK MATRIX CONSTRAINTS VIA REGISTERS
    mov ecx, [canvas_w]
    shl rcx, 2
    mov [pitch_row], rcx       ; pitch_row = Width * 4
    
    mov eax, [canvas_h]
    imul rcx, rax
    mov [pitch_total], rcx     ; pitch_total = Width * Height * 4

    ; --- THE UNIFIED TIMELINE BOOT ---
    ; 3. Setup Bitmap metadata headers based on dynamic INI vars
    ; --- FIX: Target the sub-fields via raw byte offsets instead of dotted symbols ---
    mov eax, [canvas_w]
    mov [bmi + 4], eax          ; Offset +4 skips biSize and writes biWidth
    
    mov eax, [canvas_h]
    neg eax
    mov [bmi + 8], eax          ; Offset +8 writes biHeight


    ; 4. Fetch Module Instance Context
    invoke GetModuleHandle, 0
    mov [hinstance], rax
    mov [wc.hInstance], rax

    ; 5. Initialize Arrow Cursors
    invoke LoadCursor, 0, IDC_ARROW
    mov [wc.hCursor], rax

    ; 6. Register the Window Class Profile
    invoke RegisterClassEx, wc
    test rax, rax
    jz .failed

    ; 7. Allocate continuous memory block for our Frame Buffer
    invoke GetProcessHeap
    invoke HeapAlloc, rax, 8, [pitch_total]
    mov [pixel_buffer], rax
    test rax, rax
    jz .failed

    ; -------------------------------------------------------------------------
    ; APPLICATION LOGIC LAYOUT (Values loaded cleanly into 64-bit registers)
    ; -------------------------------------------------------------------------
    ; Cache your dynamic dimensions into clean registers to pass to macros safely
    movsxd r11, dword [canvas_w]
    movsxd r12, dword [canvas_h]

    ; Clear baseline canvas space (Pass r11 width and r12 height)
    draw_rect_block pixel_buffer, r11, 0, 0, r11, r12, 0xFF1E1E2E

    ; Draw the bar on whichever edge/thickness/color the ini configures
    draw_bar pixel_buffer, r11, r12

    ; Position and stamp the close button on top of the bar
    layout_close_icon r11, r12
    movsxd r9, dword [close_btn_x]
    movsxd r10, dword [close_btn_y]
    draw_icon_masked pixel_buffer, r11, r9, r10, ICON_CLOSE, 0xFFCDD6F4

    ; Maximize / minimize, stacked inward from the close button — draw
    ; only for now, nothing hooked up to click them yet
    layout_icon_near_close 1, max_btn_x, max_btn_y
    movsxd r9, dword [max_btn_x]
    movsxd r10, dword [max_btn_y]
    draw_icon_masked pixel_buffer, r11, r9, r10, ICON_MAXIMIZE, 0xFFCDD6F4

    layout_icon_near_close 2, min_btn_x, min_btn_y
    movsxd r9, dword [min_btn_x]
    movsxd r10, dword [min_btn_y]
    draw_icon_masked pixel_buffer, r11, r9, r10, ICON_MINIMIZE, 0xFFCDD6F4

    ; App icon, opposite end of the bar from the close cluster
    layout_app_icon r11, r12
    movsxd r9, dword [app_icon_x]
    movsxd r10, dword [app_icon_y]
    draw_icon_masked pixel_buffer, r11, r9, r10, ICON_APP, 0xFFCDD6F4

    ; Max-width / max-height, centered as a pair on the bar
    layout_center_icons r11, r12
    movsxd r9, dword [max_width_x]
    movsxd r10, dword [max_width_y]
    draw_icon_masked pixel_buffer, r11, r9, r10, ICON_MAX_WIDTH, 0xFFCDD6F4

    movsxd r9, dword [max_height_x]
    movsxd r10, dword [max_height_y]
    draw_icon_masked pixel_buffer, r11, r9, r10, ICON_MAX_HEIGHT, 0xFFCDD6F4

    ; Draw your modular 100x100 Tan Square component on your dynamic coordinate grid
    draw_rect_block pixel_buffer, r11, 50, 50, 100, 100, 0xFFFAB387

    ; Draw a second box component to confirm modular rendering behavior
    draw_rect_block pixel_buffer, r11, 200, 50, 40, 40, 0xFFFAB387
    ; -------------------------------------------------------------------------

    ; 8. Mount the Borderless Canvas Overlay Window
    invoke CreateWindowEx, \
           0, _class, _title, WS_POPUP or WS_VISIBLE, \
           100, 100, [canvas_w], [canvas_h], \
           0, 0, [hinstance], 0
    test rax, rax
    jz .failed
    mov [hwnd], rax

    invoke ShowWindow, [hwnd], SW_SHOWNORMAL
    invoke UpdateWindow, [hwnd]

    ; 9. Synchronous Message Pump Loop
.msg_loop:
    invoke GetMessage, msg, 0, 0, 0
    test rax, rax
    jz .exit
    invoke TranslateMessage, msg
    invoke DispatchMessage, msg
    jmp .msg_loop

.failed:
    invoke MessageBox, 0, _error, _title, MB_OK or MB_ICONERROR
.exit:
    invoke ExitProcess, 0

section '.idata' import data readable
    library kernel32, 'KERNEL32.DLL', \
            user32,   'USER32.DLL', \
            gdi32,    'GDI32.DLL'

    include 'C:\fasm\INCLUDE\API\KERNEL32.INC'
    include 'C:\fasm\INCLUDE\API\user32.inc'
    include 'C:\fasm\INCLUDE\API\gdi32.inc'
