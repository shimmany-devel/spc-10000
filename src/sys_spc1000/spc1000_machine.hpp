#pragma once
#include <memory>
#include <string>
#include <SDL2/SDL.h>
#include "spc1000_board.hpp"
#include "../core/z80/z80_cpu.hpp"

namespace spc {

class Spc1000Machine {
public:
    static constexpr uint32_t CPU_CLOCK_HZ = 4000000; // 4.0 MHz
    static constexpr uint32_t FPS = 60;
    static constexpr uint32_t CYCLES_PER_FRAME = CPU_CLOCK_HZ / FPS; // 66,666 사이클

    Spc1000Machine();
    ~Spc1000Machine();

    bool init(const std::string& rom_path);
    void reset();

    // 1프레임(60Hz) 실행 및 VBLANK 인터럽트 처리
    void run_frame();

    // 화면 렌더링
    void render(uint32_t* framebuffer, int pitch_pixels);

    // 키보드 이벤트 전달
    void handle_key_down(SDL_Keycode sym);
    void handle_key_up(SDL_Keycode sym);

    // 카세트 테이프 제어
    bool mount_tape(const std::string& path);
    bool record_tape(const std::string& path);
    void cassette_play();
    void cassette_rec();
    void cassette_stop();

    // 시스템 스냅샷 (Save State / Load State)
    bool save_snapshot(const std::string& path = "spc1000.sav");
    bool load_snapshot(const std::string& path = "spc1000.sav");

    Spc1000Board& board() { return board_; }
    Z80Cpu& cpu() { return *cpu_; }

    uint16_t pc() const { return cpu_->pc(); }

private:
    Spc1000Board board_;
    std::unique_ptr<Z80Cpu> cpu_;
};

} // namespace spc
