#include "spc1000_machine.hpp"
#include <iostream>
#include <fstream>
#include <cstring>

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
    board_.keyboard().tick_frame();

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

bool Spc1000Machine::mount_tape(const std::string& path) {
    return board_.cassette().mount_read_tape(path);
}

bool Spc1000Machine::record_tape(const std::string& path) {
    return board_.cassette().mount_write_tape(path);
}

void Spc1000Machine::cassette_play() {
    board_.cassette().press_play();
}

void Spc1000Machine::cassette_rec() {
    board_.cassette().press_rec();
}

void Spc1000Machine::cassette_stop() {
    board_.cassette().press_stop();
}

void Spc1000Machine::cassette_rewind() {
    board_.cassette().rewind();
}

bool Spc1000Machine::save_snapshot(const std::string& path) {
    std::ofstream os(path, std::ios::binary);
    if (!os.is_open()) {
        std::cerr << "[Snapshot] Failed to create snapshot file: " << path << "\n";
        return false;
    }

    const char magic[8] = {'S', 'P', 'C', '1', 'K', 'S', 'A', 'V'};
    os.write(magic, 8);
    uint32_t version = 1;
    os.write(reinterpret_cast<const char*>(&version), sizeof(version));

    // 1. CPU
    cpu_->save_state(os);
    // 2. Board (RAM, IPLK, VDG, PSG, Cassette)
    board_.save_state(os);

    std::cout << "[Snapshot] Successfully saved state to " << path << "\n";
    return true;
}

bool Spc1000Machine::load_snapshot(const std::string& path) {
    std::ifstream is(path, std::ios::binary);
    if (!is.is_open()) {
        std::cerr << "[Snapshot] Snapshot file not found: " << path << "\n";
        return false;
    }

    char magic[8];
    is.read(magic, 8);
    if (std::memcmp(magic, "SPC1KSAV", 8) != 0) {
        std::cerr << "[Snapshot] Invalid snapshot magic in " << path << "\n";
        return false;
    }
    uint32_t version = 0;
    is.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != 1) {
        std::cerr << "[Snapshot] Unsupported snapshot version: " << version << "\n";
        return false;
    }

    // 1. CPU
    cpu_->load_state(is);
    // 2. Board (RAM, IPLK, VDG, PSG, Cassette)
    board_.load_state(is);

    std::cout << "[Snapshot] Successfully restored state from " << path 
              << " (PC: 0x" << std::hex << cpu_->pc() << std::dec << ")\n";
    return true;
}

} // namespace spc
