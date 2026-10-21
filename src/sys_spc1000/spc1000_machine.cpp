#include "spc1000_machine.hpp"
#include <iostream>

namespace spc {

Spc1000Machine::Spc1000Machine() {
    cpu_ = std::make_unique<Z80Cpu>(board_);
}

Spc1000Machine::~Spc1000Machine() = default;

bool Spc1000Machine::init(const std::string& rom_path) {
    if (!board_.load_rom(rom_path)) {
        return false;
    }
    reset();
    return true;
}

void Spc1000Machine::reset() {
    board_.reset();
    cpu_->reset();
}

void Spc1000Machine::run_frame() {
    // 1프레임 동안 Z80 CPU 실행 (66,666 사이클)
    // 수직 블랭킹 전후로 인터럽트 시점 모사
    constexpr uint32_t ACTIVE_CYCLES = (CYCLES_PER_FRAME * 5) / 6;
    constexpr uint32_t VBLANK_CYCLES = CYCLES_PER_FRAME - ACTIVE_CYCLES;

    cpu_->step_cycles(ACTIVE_CYCLES);

    // VBLANK 인터럽트 발생
    cpu_->trigger_irq();

    cpu_->step_cycles(VBLANK_CYCLES);
}

void Spc1000Machine::render(uint32_t* framebuffer, int pitch_pixels) {
    board_.vdg().render(framebuffer, pitch_pixels);
}

void Spc1000Machine::handle_key_down(SDL_Keycode sym) {
    board_.keyboard().handle_key_down(sym);
}

void Spc1000Machine::handle_key_up(SDL_Keycode sym) {
    board_.keyboard().handle_key_up(sym);
}

} // namespace spc
