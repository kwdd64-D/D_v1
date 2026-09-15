format PE64 GUI 5.0
entry start

; Resolved via FASM's own include search path (the INCLUDE environment
; variable). build.bat sets this for you before invoking fasm, so this
; line works regardless of where FASM is installed on your machine.
include 'win64w.inc'

section '.data' data readable writeable
    _class        TCHAR 'NakedCanvasClass', 0
    _title        TCHAR 'Workspace', 0
    _error        TCHAR 'Window startup failed.', 0
    
    ; --- Config path is resolved at runtime, relative to this .exe's own
    ; folder (see the "Locate config.ini" block in start:), so the build
    ; doesn't need to live at any particular drive/folder on disk.
    _ini_path     rb 520          ; MAX_PATH (260) WCHARs, filled in at startup
    _ini_suffix   du '..\config\config.ini', 0
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
; Order matters here: icon_ids.inc/icon.inc must come before titlebar.inc
; (its close-button macros use ICON_CELL), and titlebar.inc must come
; before win64_host.inc (its WM_NCHITTEST/WM_LBUTTONDOWN handlers invoke
; titlebar.inc's macros, which need to already be defined).
include '..\lib\icon_ids.inc'
include '..\lib\icon.inc'
include '..\lib\titlebar.inc'
include '..\lib\win64_host.inc'
include '..\lib\draw2d.inc'

section '.text' code readable executable
start:
    sub rsp, 40 ; Maintain rigid 16-byte shadow stack alignment boundaries

    ; 0. Locate config.ini relative to THIS executable's own folder, so the
    ; build works regardless of which drive/folder the repo was cloned into.
    ; _ini_path ends up as ".....\src\..\config\config.ini", which Windows
    ; resolves fine even with the ".." in it.
    invoke GetModuleFileNameW, 0, _ini_path, 260
    mov rcx, rax                    ; rcx = index just past the last char
.find_slash:
    test rcx, rcx
    jz .slash_done                  ; safety net: no backslash found at all
    dec rcx
    movzx eax, word [_ini_path + rcx*2]
    cmp eax, '\'
    jne .find_slash
    inc rcx                         ; keep the slash, resume writing after it
.slash_done:
    lea rdi, [_ini_path + rcx*2]
    lea rsi, [_ini_suffix]
.copy_suffix:
    movzx eax, word [rsi]
    mov [rdi], ax
    add rsi, 2
    add rdi, 2
    test eax, eax
    jnz .copy_suffix

    ; 1. SUCK THE LIVE INI VARIABLES FROM DISK NATIVELY INTO REGISTERS
    invoke GetPrivateProfileIntW, _ini_section, _ini_width, 800, _ini_path
    mov [canvas_w], eax

    invoke GetPrivateProfileIntW, _ini_section, _ini_height, 600, _ini_path
    mov [canvas_h], eax

    ; Load [TitleBar] Location/Thickness/Color from the same ini file
    init_titlebar

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

    ; Draw the title bar on whichever edge/thickness/color the ini configures
    draw_titlebar pixel_buffer, r11, r12

    ; Position and stamp the close button on top of the bar
    layout_close_icon r11, r12
    movsxd r9, dword [close_btn_x]
    movsxd r10, dword [close_btn_y]
    draw_icon_masked pixel_buffer, r11, r9, r10, ICON_CLOSE, 0xFFCDD6F4

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

    include 'API\KERNEL32.INC'
    include 'API\USER32.INC'
    include 'API\GDI32.INC'
