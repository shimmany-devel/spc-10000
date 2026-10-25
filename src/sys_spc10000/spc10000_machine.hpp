#pragma once
#include <memory>
#include <string>
#include <SDL2/SDL.h>
#include "spc10000_board.hpp"
#include "../core/z8000/z8000_cpu.hpp"

namespace spc {

class Spc10000Machine {
public:
    static constexpr uint32_t CPU_CLOCK_HZ = 10000000; // 10.0 MHz
    static constexpr uint32_t FPS = 60;
    static constexpr uint32_t CYCLES_PER_FRAME = CPU_CLOCK_HZ / FPS; // 166,666 사이클

    Spc10000Machine();
    ~Spc10000Machine();

    bool init(const std::string& rom_path);
    void reset();

    // 1프레임(60Hz) 실행
    void run_frame();

    // 화면 렌더링 (레거시 MC6847 또는 256색 슈퍼 그래픽)
    void render(uint32_t* framebuffer, int pitch_pixels);

    // 키보드 이벤트
    void handle_key_down(SDL_Keycode sym);
    void handle_key_up(SDL_Keycode sym);

    // 카세트 테이프 제어 (UI 호환 인터페이스)
    bool mount_tape(const std::string& path);
    bool record_tape(const std::string& path);
    void cassette_play();
    void cassette_rec();
    void cassette_stop();
    void cassette_rewind() {}
    bool is_cassette_motor_on() const { return false; }

    // 시스템 스냅샷 (Save State / Load State)
    bool save_snapshot(const std::string& path = "spc10000.sav");
    bool load_snapshot(const std::string& path = "spc10000.sav");

    Spc10000Board& board() { return board_; }
    Z8000Cpu& cpu() { return *cpu_; }

    uint32_t pc() const { return cpu_->regs().physical_pc(); }

private:
    Spc10000Board board_;
    std::unique_ptr<Z8000Cpu> cpu_;
};

} // namespace spc
