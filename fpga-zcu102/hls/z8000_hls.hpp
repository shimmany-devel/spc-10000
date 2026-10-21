#pragma once
#include <stdint.h>

struct Z8000_Registers {
    uint16_t r[16];
    uint16_t pc_offset;
    uint8_t  pc_seg;
    uint16_t fcw;
};

// Top-level function for Vitis HLS synthesis
void z8000_hls_step(
    Z8000_Registers *regs,
    uint16_t mem_in,
    uint16_t *mem_out,
    uint32_t *mem_addr,
    bool *mreq,
    bool *iorq,
    bool *wr,
    bool *rd,
    uint8_t *byte_en
);
