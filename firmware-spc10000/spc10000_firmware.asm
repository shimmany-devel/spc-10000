; ==============================================================================
; SPC-10000 16-bit Native Firmware (ROM: 0x0000 - 0x7FFF, 32KB)
; Target Architecture: Zilog Z8001 (Segmented, Big-Endian)
; System: SPC-10000 (Z8001 @ 10MHz, 256KB RAM, MC6847 VDG + Super Graphics)
; ==============================================================================

; --- Program Status Area (PSA) at Segment 0, Offset 0x0000 ---
; Z8001 Hardware Reset Vector Table:
; 0x0000: Reserved (0x0000)
; 0x0002: Initial FCW (0xC000 = Segmented + System Mode)
; 0x0004: Initial PC Segment (0x0000)
; 0x0006: Initial PC Offset (0x0100)
; -------------------------------------------------------------

.org 0x0000
    .word 0x0000        ; Reserved
    .word 0xC000        ; FCW: Segmented Mode + System Mode
    .word 0x0000        ; PC Segment: 0
    .word 0x0100        ; PC Offset: 0x0100 (Bootstrap Entry)

; Trap & Interrupt Vectors (0x0008 - 0x001F)
    .word 0xC000, 0x0000, 0x0100    ; EPU Trap
    .word 0xC000, 0x0000, 0x0100    ; Privilege Trap
    .word 0xC000, 0x0000, 0x0100    ; Syscall
    .word 0xC000, 0x0000, 0x0100    ; SegTrap
    .word 0xC000, 0x0000, 0x0100    ; NMI
    .word 0xC000, 0x0000, 0x0100    ; NVI
    .word 0xC000, 0x0000, 0x0100    ; VI

; --- Bootstrap Entry Point at Offset 0x0100 ---
.org 0x0100
bootstrap:
    ; 1. 스택 포인터 초기화 (RR14 = Segment 0, Offset 0xFFFE)
    LD  R14, 0x0000
    LD  R15, 0xFFFE

    ; 2. 하드웨어 리셋: VDG GMODE = 0 (Text Mode), Cassette = 0
    LDB RL1, 0x00
    OUTB @0x2000, RL1
    OUTB @0x6000, RL1

    ; 3. VRAM 텍스트 화면 클리어 (0x0000 ~ 0x01FF 에 공백 0x20 쓰기)
    ;    R1 = VRAM 포트 주소 (0x0000), R2 = 카운트 (512), RL3 = 공백 문자 (' ')
    LD  R1, 0x0000
    LD  R2, 512
    LDB RL3, 0x20

clear_vram_loop:
    OUTB @0x0000, RL3   ; Note: 포트 주소 매핑
    DEC R2
    JR  NZ, clear_vram_loop

    ; 4. 부팅 배너 및 안내 메시지 출력
    ;    R1 = VRAM 출력 위치 (행 1, 열 2 = offset 34)
    ;    (문자열 출력 루틴 호출)
    
    ; 5. 키보드 스캔 루프로 진입
idle_prompt_loop:
    INB RL0, @0x8001    ; Read keyboard row 1
    HALT
    JR  idle_prompt_loop
