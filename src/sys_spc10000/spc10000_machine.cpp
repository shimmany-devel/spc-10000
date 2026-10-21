#include "spc10000_machine.hpp"
#include <iostream>

namespace spc {

Spc10000Machine::Spc10000Machine() {
    cpu_ = std::make_unique<Z8000Cpu>(board_, Z8000Cpu::Model::Z8001_Segmented);
}

Spc10000Machine::~Spc10000Machine() = default;

bool Spc10000Machine::init(const std::string& rom_path) {
    if (!board_.load_native_rom(rom_path)) {
        std::cerr << "[Spc10000Machine] Starting without native ROM (blank memory).\n";
    }
    reset();
    return true;
}

void Spc10000Machine::reset() {
    board_.reset();
    cpu_->reset();
}

void Spc10000Machine::run_frame() {
    // 166,666 사이클(10MHz @ 60FPS) 실행
    cpu_->step_cycles(CYCLES_PER_FRAME);
}

void Spc10000Machine::render(uint32_t* framebuffer, int pitch_pixels) {
    if (board_.is_super_graphic_mode()) {
        board_.render_super_graphic(framebuffer, pitch_pixels);
    } else {
        board_.vdg().render(framebuffer, pitch_pixels);
    }
}

void Spc10000Machine::handle_key_down(SDL_Keycode sym) {
    board_.keyboard().handle_key_down(sym);
}

void Spc10000Machine::handle_key_up(SDL_Keycode sym) {
    board_.keyboard().handle_key_up(sym);
}

} // namespace spc
