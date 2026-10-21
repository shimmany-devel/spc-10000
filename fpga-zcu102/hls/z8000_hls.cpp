#include "z8000_hls.hpp"

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
) {
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE ap_none port=mem_in
#pragma HLS INTERFACE ap_none port=mem_out
#pragma HLS INTERFACE ap_none port=mem_addr
#pragma HLS INTERFACE ap_none port=mreq
#pragma HLS INTERFACE ap_none port=iorq
#pragma HLS INTERFACE ap_none port=wr
#pragma HLS INTERFACE ap_none port=rd
#pragma HLS INTERFACE ap_none port=byte_en

    uint32_t pc = ((uint32_t)regs->pc_seg << 16) | regs->pc_offset;
    uint16_t op = mem_in;

    *mreq = true;
    *iorq = false;
    *wr = false;
    *rd = true;
    *byte_en = 0x03;
    *mem_addr = pc;

    // Decode basic opcodes
    uint8_t op_high = op >> 8;
    if ((op_high & 0xF0) == 0x20) {
        // LD Rn, imm16
        uint8_t r = op & 0x0F;
        regs->r[r] = mem_in; // simplified pipeline
        regs->pc_offset += 2;
    } else if (op_high == 0x01) {
        // ADD R_dst, R_src
        uint8_t rd_idx = (op >> 4) & 0x0F;
        uint8_t rs_idx = op & 0x0F;
        regs->r[rd_idx] += regs->r[rs_idx];
        regs->pc_offset += 2;
    } else {
        regs->pc_offset += 2;
    }
}
