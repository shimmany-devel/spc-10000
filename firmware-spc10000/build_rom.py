#!/usr/bin/env python3
"""
SPC-10000 Native 16-bit ROM Generator (Fixed Instruction Word Encoding)
Target Architecture: Zilog Z8001 (Segmented, Big-Endian)
ROM Size: 32KB (0x0000 - 0x7FFF)
"""

import struct

def build_spc10000_rom():
    ROM_SIZE = 32768
    rom = bytearray([0x00] * ROM_SIZE)

    # 1. Program Status Area (PSA) at 0x0000 - 0x001F
    # 0x00: Reserved (0x0000)
    rom[0x00] = 0x00
    rom[0x01] = 0x00
    # 0x02: Initial FCW (0xC000: Segmented Mode + System Mode)
    rom[0x02] = 0xC0
    rom[0x03] = 0x00
    # 0x04: Initial PC Segment (0x0000)
    rom[0x04] = 0x00
    rom[0x05] = 0x00
    # 0x06: Initial PC Offset (0x0100)
    rom[0x06] = 0x01
    rom[0x07] = 0x00

    # 2. Bootstrap Code at Offset 0x0100
    code = bytearray()

    # LD R14, 0x0000 (0x210E, 0x0000) - Segment 0
    code += struct.pack(">HH", 0x210E, 0x0000)
    # LD R15, 0xFFFE (0x210F, 0xFFFE) - Stack Offset
    code += struct.pack(">HH", 0x210F, 0xFFFE)
    # LDB RL1, 0x00  (0x0C03, 0x0000) - RL1 is reg 3
    code += struct.pack(">HH", 0x0C03, 0x0000)
    # OUTB @0x2000, RL1 (0x3B03, 0x2000) -> GMODE = 0 (Text mode)
    code += struct.pack(">HH", 0x3B03, 0x2000)
    # OUTB @0x6000, RL1 (0x3B03, 0x6000) -> Cassette Motor = 0
    code += struct.pack(">HH", 0x3B03, 0x6000)

    # 배너 텍스트 문자열들을 VRAM 포트(0x0000~)에 출력하는 머신 코드:
    banner_lines = [
        (1,  " ** SPC-10000 SYSTEM MONITOR ** "),
        (2,  " Z8001 CPU @ 10MHz / 256KB RAM  "),
        (3,  " SUPER Hu-BASIC VERSION 2.0     "),
        (4,  " (C) 1983, 2026 SAMSUNG / AI    "),
        (6,  "Ready                           "),
        (7,  "_                               ")
    ]

    for row, text in banner_lines:
        start_vram = row * 32
        for i, char in enumerate(text):
            vram_port = start_vram + i
            char_byte = ord(char)
            # LDB RL1, char_byte (0x0C03, char_byte)
            code += struct.pack(">HH", 0x0C03, char_byte)
            # OUTB @vram_port, RL1 (0x3B03, vram_port)
            code += struct.pack(">HH", 0x3B03, vram_port)

    # 속성 버퍼(0x0800 ~) Line 1 강조 속성 (Attribute = 1, 반전 텍스트)
    for i in range(32):
        attr_port = 0x0800 + 32 + i
        code += struct.pack(">HH", 0x0C03, 0x0001)
        code += struct.pack(">HH", 0x3B03, attr_port)

    # 무한 대기 / 키보드 폴링 루프
    loop_start_offset = 0x0100 + len(code)
    # INB RL0, @0x8001 (0x3A01, 0x8001) -> RL0 is reg 1
    code += struct.pack(">HH", 0x3A01, 0x8001)
    # JP T, loop_start_offset (0x5E80, loop_start_offset)
    code += struct.pack(">HH", 0x5E80, loop_start_offset)

    # 코드를 0x0100 오프셋에 복사
    rom[0x0100:0x0100+len(code)] = code

    output_path = "firmware-spc10000/spc10000.rom"
    with open(output_path, "wb") as f:
        f.write(rom)

    print(f"[ROM Builder] Generated {len(rom)} bytes SPC-10000 Native ROM at {output_path}")
    print(f"[ROM Builder] Bootstrap code size: {len(code)} bytes (0x0100 - 0x{0x0100+len(code):04X})")

if __name__ == "__main__":
    build_spc10000_rom()
