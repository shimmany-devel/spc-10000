#include "z80_cpu.hpp"
#include <iostream>

#define CHIPS_IMPL
#include "../../../reference/chips-repo/chips/z80.h"

namespace spc {

struct Z80Cpu::Impl {
    IBus& bus;
    z80_t cpu{};
    uint64_t pins{0};
    uint32_t irq_pulse_cycles{0};

    explicit Impl(IBus& b) : bus(b) {
        pins = z80_init(&cpu);
    }

    void reset() {
        pins = z80_reset(&cpu);
        irq_pulse_cycles = 0;
    }

    void prefetch(uint16_t pc) {
        pins = z80_prefetch(&cpu, pc);
    }

    uint64_t tick_single(uint64_t p) {
        if (irq_pulse_cycles > 0) {
            irq_pulse_cycles--;
            if (irq_pulse_cycles == 0) {
                p &= ~Z80_INT;
            }
        }

        p = z80_tick(&cpu, p);
        const uint16_t addr = Z80_GET_ADDR(p);

        if (p & Z80_MREQ) {
            if (p & Z80_RD) {
                uint8_t data = bus.mem_read(addr);
                Z80_SET_DATA(p, data);
            } else if (p & Z80_WR) {
                uint8_t data = Z80_GET_DATA(p);
                bus.mem_write(addr, data);
            }
        } else if (p & Z80_IORQ) {
            if (p & Z80_M1) {
                // Interrupt Acknowledge: clear INT line and supply RST 38h
                Z80_SET_DATA(p, 0xFF);
                p &= ~Z80_INT;
                irq_pulse_cycles = 0;
            } else if (p & Z80_RD) {
                uint8_t data = bus.io_read(addr);
                Z80_SET_DATA(p, data);
            } else if (p & Z80_WR) {
                uint8_t data = Z80_GET_DATA(p);
                bus.io_write(addr, data);
            }
        }
        return p;
    }

    uint32_t step_cycles(uint32_t target_cycles) {
        uint32_t elapsed = 0;
        while (elapsed < target_cycles) {
            pins = tick_single(pins);
            elapsed++;
        }
        return elapsed;
    }

    uint32_t step_instruction() {
        uint32_t cycles = 0;
        do {
            pins = tick_single(pins);
            cycles++;
        } while (!z80_opdone(&cpu) && cycles < 10000);
        return cycles;
    }
};

Z80Cpu::Z80Cpu(IBus& bus) : pimpl_(std::make_unique<Impl>(bus)) {}
Z80Cpu::~Z80Cpu() = default;

void Z80Cpu::reset() {
    pimpl_->reset();
}

void Z80Cpu::prefetch(uint16_t pc) {
    pimpl_->prefetch(pc);
}

uint32_t Z80Cpu::step_cycles(uint32_t target_cycles) {
    return pimpl_->step_cycles(target_cycles);
}

uint32_t Z80Cpu::step_instruction() {
    return pimpl_->step_instruction();
}

void Z80Cpu::trigger_irq() {
    // Assert INT pin with hardware pulse width (64 T-cycles)
    pimpl_->pins |= Z80_INT;
    pimpl_->irq_pulse_cycles = 64;
}

uint16_t Z80Cpu::pc() const {
    return pimpl_->cpu.pc;
}

uint16_t Z80Cpu::sp() const {
    return pimpl_->cpu.sp;
}

void Z80Cpu::save_state(std::ostream& os) {
    os.write(reinterpret_cast<const char*>(&pimpl_->cpu), sizeof(pimpl_->cpu));
    os.write(reinterpret_cast<const char*>(&pimpl_->pins), sizeof(pimpl_->pins));
}

void Z80Cpu::load_state(std::istream& is) {
    is.read(reinterpret_cast<char*>(&pimpl_->cpu), sizeof(pimpl_->cpu));
    is.read(reinterpret_cast<char*>(&pimpl_->pins), sizeof(pimpl_->pins));
}

} // namespace spc
