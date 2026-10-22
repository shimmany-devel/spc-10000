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

bool Spc10000Machine::mount_tape(const std::string& path) {
    std::cout << "[Spc10000Machine] Tape mount not supported on SPC-10000: " << path << "\n";
    return false;
}

bool Spc10000Machine::record_tape(const std::string& path) {
    std::cout << "[Spc10000Machine] Tape record not supported on SPC-10000: " << path << "\n";
    return false;
}

void Spc10000Machine::cassette_play() {
    std::cout << "[Spc10000Machine] Tape PLAY (not implemented on SPC-10000)\n";
}

void Spc10000Machine::cassette_rec() {
    std::cout << "[Spc10000Machine] Tape REC (not implemented on SPC-10000)\n";
}

void Spc10000Machine::cassette_stop() {
    std::cout << "[Spc10000Machine] Tape STOP (not implemented on SPC-10000)\n";
}

bool Spc10000Machine::save_snapshot(const std::string& path) {
    std::cout << "[Spc10000Machine] Snapshot save not yet implemented on SPC-10000: " << path << "\n";
    return false;
}

bool Spc10000Machine::load_snapshot(const std::string& path) {
    std::cout << "[Spc10000Machine] Snapshot load not yet implemented on SPC-10000: " << path << "\n";
    return false;
}

} // namespace spc
