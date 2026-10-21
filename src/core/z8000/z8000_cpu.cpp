#include "z8000_cpu.hpp"
#include <iostream>

namespace spc {

struct Z8000Cpu::Impl {
    IZ8000Bus& bus;
    Model model;
    Z8000Registers regs{};
    bool halted{false};

    Impl(IZ8000Bus& b, Model m) : bus(b), model(m) {
        reset();
    }

    void reset() {
        regs = Z8000Registers{};
        if (model == Model::Z8001_Segmented) {
            regs.fcw = FCW_SEG | FCW_S_N; // Z8001 Segmented + System Mode
        } else {
            regs.fcw = FCW_S_N;           // Z8002 Non-segmented + System Mode
        }

        // Program Status Area(PSA)의 리셋 벡터(0x000000)에서 FCW 및 PC 로드
        // Z8001 리셋 벡터 구조:
        //   Addr 0x00: Reserved (0x0000)
        //   Addr 0x02: Initial FCW
        //   Addr 0x04: Initial PC Segment (word, MSB is segment)
        //   Addr 0x06: Initial PC Offset (word)
        // 만약 PSA가 아직 비어있다면 기본값 0x000000으로 부팅 시작
        uint16_t init_fcw = bus.mem_read_word(0x0002);
        uint16_t init_seg = bus.mem_read_word(0x0004);
        uint16_t init_off = bus.mem_read_word(0x0006);

        if (init_fcw != 0xFFFF && init_fcw != 0x0000) {
            regs.fcw = init_fcw;
            regs.pc_seg = static_cast<uint8_t>(init_seg >> 8);
            regs.pc_off = init_off;
        } else {
            regs.pc_seg = 0;
            regs.pc_off = 0x0000;
        }

        // 시스템 스택 포인터 초기화 (Segment 0, 0xFFFE)
        regs.r[14] = 0x0000;
        regs.r[15] = 0xFFFE;
        regs.r14_sys = 0x0000;
        regs.r15_sys = 0xFFFE;
        halted = false;
    }

    uint32_t current_pc_linear() const {
        if (regs.fcw & FCW_SEG) {
            return (static_cast<uint32_t>(regs.pc_seg & 0x7F) << 16) | regs.pc_off;
        }
        return regs.pc_off;
    }

    uint16_t fetch_word() {
        uint32_t addr = current_pc_linear();
        uint16_t word = bus.mem_read_word(addr);
        regs.pc_off += 2;
        return word;
    }

    void push_word(uint16_t val) {
        regs.r[15] -= 2;
        uint32_t sp_addr = (regs.fcw & FCW_SEG)
            ? ((static_cast<uint32_t>(regs.r[14] & 0x7F) << 16) | regs.r[15])
            : regs.r[15];
        bus.mem_write_word(sp_addr, val);
    }

    uint16_t pop_word() {
        uint32_t sp_addr = (regs.fcw & FCW_SEG)
            ? ((static_cast<uint32_t>(regs.r[14] & 0x7F) << 16) | regs.r[15])
            : regs.r[15];
        uint16_t val = bus.mem_read_word(sp_addr);
        regs.r[15] += 2;
        return val;
    }

    // 조건 코드 검사 (Z8000 Condition Codes: cc = 0~15)
    bool check_condition(uint8_t cc) const {
        const bool c = (regs.fcw & FCW_C) != 0;
        const bool z = (regs.fcw & FCW_Z) != 0;
        const bool s = (regs.fcw & FCW_S) != 0;
        const bool pv = (regs.fcw & FCW_PV) != 0;

        switch (cc & 0x0F) {
            case 0x0: return false;       // F (Never)
            case 0x1: return !s && !z;    // GT (Greater Than)
            case 0x2: return !c && !z;    // UGTE / HI (Higher)
            case 0x3: return pv;          // OV (Overflow)
            case 0x4: return !pv;         // NOV (No Overflow)
            case 0x5: return c;           // C / ULT (Carry / Lower)
            case 0x6: return z;           // Z / EQ (Zero / Equal)
            case 0x7: return s;           // MI / NEG (Minus)
            case 0x8: return true;        // T (Always)
            case 0x9: return s || z;      // LE (Less Than or Equal)
            case 0xA: return c || z;      // ULE / LOS (Lower or Same)
            case 0xB: return !pv;         // PE (Parity Even)
            case 0xC: return pv;          // PO (Parity Odd)
            case 0xD: return !c;          // NC / UGE (No Carry)
            case 0xE: return !z;          // NZ / NE (Not Zero)
            case 0xF: return !s;          // PL / POS (Plus)
            default: return true;
        }
    }

    void update_flags_logic16(uint16_t res) {
        regs.fcw &= ~(FCW_C | FCW_Z | FCW_S | FCW_PV);
        if (res == 0) regs.fcw |= FCW_Z;
        if (res & 0x8000) regs.fcw |= FCW_S;
        // Parity
        uint8_t p = static_cast<uint8_t>(res ^ (res >> 8));
        p ^= (p >> 4);
        p ^= (p >> 2);
        p ^= (p >> 1);
        if (!(p & 1)) regs.fcw |= FCW_PV;
    }

    void update_flags_add16(uint16_t a, uint16_t b, uint32_t sum) {
        regs.fcw &= ~(FCW_C | FCW_Z | FCW_S | FCW_PV | FCW_DA | FCW_H);
        uint16_t res = static_cast<uint16_t>(sum);
        if (sum > 0xFFFF) regs.fcw |= FCW_C;
        if (res == 0) regs.fcw |= FCW_Z;
        if (res & 0x8000) regs.fcw |= FCW_S;
        if (~(a ^ b) & (a ^ res) & 0x8000) regs.fcw |= FCW_PV; // Overflow
    }

    void update_flags_sub16(uint16_t a, uint16_t b, uint32_t diff) {
        regs.fcw &= ~(FCW_C | FCW_Z | FCW_S | FCW_PV | FCW_DA | FCW_H);
        regs.fcw |= FCW_DA;
        uint16_t res = static_cast<uint16_t>(diff);
        if (a < b) regs.fcw |= FCW_C;
        if (res == 0) regs.fcw |= FCW_Z;
        if (res & 0x8000) regs.fcw |= FCW_S;
        if ((a ^ b) & (a ^ res) & 0x8000) regs.fcw |= FCW_PV;
    }

    // 단일 명령어 실행
    uint32_t step_instruction() {
        if (halted) return 4;

        uint16_t op = fetch_word();
        uint32_t cycles = 6;

        // 1. NOP (0x8D07)
        if (op == 0x8D07) {
            return 4;
        }

        // 2. HALT (0x7A00)
        if (op == 0x7A00) {
            halted = true;
            return 8;
        }

        // 3. LD r, imm16 (0x2100 | (r << 8)) -> 0x21r0
        if ((op & 0xF00F) == 0x2100) {
            uint8_t r = (op >> 8) & 0x0F;
            uint16_t imm = fetch_word();
            regs.r[r] = imm;
            return 7;
        }

        // 4. LDB rb, imm8 (0x0C00 | (rb << 8) | imm8)
        if ((op & 0xF000) == 0x0000 && ((op >> 8) & 0x0F) == 0x0C) {
            uint8_t rb = (op >> 4) & 0x0F;
            uint8_t imm = static_cast<uint8_t>(op & 0xFF);
            regs.set_rb(rb, imm);
            return 7;
        }

        // 5. LDL rr, imm32 (0x1400 | (rr << 8))
        if ((op & 0xF00F) == 0x1400) {
            uint8_t rr = (op >> 8) & 0x0E;
            uint16_t high = fetch_word();
            uint16_t low = fetch_word();
            regs.set_rr(rr, (static_cast<uint32_t>(high) << 16) | low);
            return 11;
        }

        // 6. LD r, r' (0x1000 | (r << 8) | (r' << 4))
        if ((op & 0xF00F) == 0x1000) {
            uint8_t dst = (op >> 8) & 0x0F;
            uint8_t src = (op >> 4) & 0x0F;
            regs.r[dst] = regs.r[src];
            return 3;
        }

        // 7. LDB rb, rb' (0x0000 | (rb << 8) | (rb' << 4))
        if ((op & 0xF00F) == 0x0000) {
            uint8_t dst = (op >> 8) & 0x0F;
            uint8_t src = (op >> 4) & 0x0F;
            regs.set_rb(dst, regs.get_rb(src));
            return 3;
        }

        // 8. ADD r, imm16 (0x0100 | (r << 8))
        if ((op & 0xF00F) == 0x0100) {
            uint8_t r = (op >> 8) & 0x0F;
            uint16_t imm = fetch_word();
            uint32_t sum = static_cast<uint32_t>(regs.r[r]) + imm;
            update_flags_add16(regs.r[r], imm, sum);
            regs.r[r] = static_cast<uint16_t>(sum);
            return 7;
        }

        // 9. SUB r, imm16 (0x0200 | (r << 8))
        if ((op & 0xF00F) == 0x0200) {
            uint8_t r = (op >> 8) & 0x0F;
            uint16_t imm = fetch_word();
            uint32_t diff = static_cast<uint32_t>(regs.r[r]) - imm;
            update_flags_sub16(regs.r[r], imm, diff);
            regs.r[r] = static_cast<uint16_t>(diff);
            return 7;
        }

        // 10. CP r, imm16 (0x0300 | (r << 8))
        if ((op & 0xF00F) == 0x0300) {
            uint8_t r = (op >> 8) & 0x0F;
            uint16_t imm = fetch_word();
            uint32_t diff = static_cast<uint32_t>(regs.r[r]) - imm;
            update_flags_sub16(regs.r[r], imm, diff);
            return 7;
        }

        // 11. JP cc, dst (0x5E00 | (cc << 4) / 0x1E00)
        // Non-segmented: 0x5E00 + cc (offset16)
        // Segmented: 0x1E00 + cc (seg16 + off16)
        if ((op & 0xFF00) == 0x5E00) {
            uint8_t cc = (op >> 4) & 0x0F;
            uint16_t target_off = fetch_word();
            if (check_condition(cc)) {
                regs.pc_off = target_off;
                cycles += 4;
            }
            return cycles;
        }
        if ((op & 0xFF00) == 0x1E00) {
            uint8_t cc = (op >> 4) & 0x0F;
            uint16_t target_seg = fetch_word();
            uint16_t target_off = fetch_word();
            if (check_condition(cc)) {
                regs.pc_seg = static_cast<uint8_t>(target_seg >> 8);
                regs.pc_off = target_off;
                cycles += 6;
            }
            return cycles;
        }

        // 12. CALL dst (0x5F00 / 0x1F00)
        if (op == 0x5F00) {
            uint16_t target_off = fetch_word();
            push_word(regs.pc_off);
            regs.pc_off = target_off;
            return 12;
        }
        if (op == 0x1F00) {
            uint16_t target_seg = fetch_word();
            uint16_t target_off = fetch_word();
            push_word(regs.pc_off);
            push_word(static_cast<uint16_t>(regs.pc_seg) << 8);
            regs.pc_seg = static_cast<uint8_t>(target_seg >> 8);
            regs.pc_off = target_off;
            return 18;
        }

        // 13. RET cc (0x9E00 | (cc << 4))
        if ((op & 0xFF00) == 0x9E00) {
            uint8_t cc = (op >> 4) & 0x0F;
            if (check_condition(cc)) {
                if (regs.fcw & FCW_SEG) {
                    uint16_t seg_w = pop_word();
                    regs.pc_seg = static_cast<uint8_t>(seg_w >> 8);
                }
                regs.pc_off = pop_word();
                return 12;
            }
            return 5;
        }

        // 14. PUSH @SP, r (0x9300 | (r << 8))
        if ((op & 0xF0FF) == 0x9300) {
            uint8_t r = (op >> 8) & 0x0F;
            push_word(regs.r[r]);
            return 10;
        }

        // 15. POP r, @SP (0x9200 | (r << 8))
        if ((op & 0xF0FF) == 0x9200) {
            uint8_t r = (op >> 8) & 0x0F;
            regs.r[r] = pop_word();
            return 8;
        }

        // 16. INB rb, @port (0x3A00 | (rb << 8))
        if ((op & 0xF0FF) == 0x3A00) {
            uint8_t rb = (op >> 8) & 0x0F;
            uint16_t port = fetch_word();
            uint8_t val = bus.io_read_byte(port);
            regs.set_rb(rb, val);
            return 11;
        }

        // 17. OUTB @port, rb (0x3B00 | (rb << 8))
        if ((op & 0xF0FF) == 0x3B00) {
            uint8_t rb = (op >> 8) & 0x0F;
            uint16_t port = fetch_word();
            bus.io_write_byte(port, regs.get_rb(rb));
            return 11;
        }

        // 18. IN r, @port (0x3C00 | (r << 8))
        if ((op & 0xF0FF) == 0x3C00) {
            uint8_t r = (op >> 8) & 0x0F;
            uint16_t port = fetch_word();
            uint16_t val = bus.io_read_word(port);
            regs.r[r] = val;
            return 11;
        }

        // 19. OUT @port, r (0x3D00 | (r << 8))
        if ((op & 0xF0FF) == 0x3D00) {
            uint8_t r = (op >> 8) & 0x0F;
            uint16_t port = fetch_word();
            bus.io_write_word(port, regs.r[r]);
            return 11;
        }

        // 20. Special I/O (SINB: 0x3800, SOUTB: 0x3900)
        if ((op & 0xF0FF) == 0x3800) {
            uint8_t rb = (op >> 8) & 0x0F;
            uint16_t port = fetch_word();
            regs.set_rb(rb, bus.special_io_read(port));
            return 12;
        }
        if ((op & 0xF0FF) == 0x3900) {
            uint8_t rb = (op >> 8) & 0x0F;
            uint16_t port = fetch_word();
            bus.special_io_write(port, regs.get_rb(rb));
            return 12;
        }

        // Unimplemented / Generic instruction fallback
        return 6;
    }

    uint32_t step_cycles(uint32_t target_cycles) {
        uint32_t elapsed = 0;
        while (elapsed < target_cycles && !halted) {
            elapsed += step_instruction();
        }
        return elapsed;
    }
};

Z8000Cpu::Z8000Cpu(IZ8000Bus& bus, Model model)
    : pimpl_(std::make_unique<Impl>(bus, model)), model_(model) {}

Z8000Cpu::~Z8000Cpu() = default;

void Z8000Cpu::reset() {
    pimpl_->reset();
}

void Z8000Cpu::set_pc(uint8_t seg, uint16_t off) {
    pimpl_->regs.pc_seg = seg;
    pimpl_->regs.pc_off = off;
}

void Z8000Cpu::set_pc_linear(uint32_t addr) {
    pimpl_->regs.pc_seg = static_cast<uint8_t>((addr >> 16) & 0x7F);
    pimpl_->regs.pc_off = static_cast<uint16_t>(addr & 0xFFFF);
}

uint32_t Z8000Cpu::step_instruction() {
    return pimpl_->step_instruction();
}

uint32_t Z8000Cpu::step_cycles(uint32_t target_cycles) {
    return pimpl_->step_cycles(target_cycles);
}

void Z8000Cpu::trigger_nmi() {
    // Non-maskable interrupt trap
}

void Z8000Cpu::trigger_vi(uint16_t vector) {
    // Vectored interrupt
}

void Z8000Cpu::trigger_nvi() {
    // Non-vectored interrupt
}

const Z8000Registers& Z8000Cpu::regs() const {
    return pimpl_->regs;
}

Z8000Registers& Z8000Cpu::regs() {
    return pimpl_->regs;
}

bool Z8000Cpu::is_segmented() const {
    return (pimpl_->regs.fcw & FCW_SEG) != 0;
}

} // namespace spc
