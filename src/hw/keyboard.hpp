#pragma once
#include <cstdint>
#include <array>
#include <unordered_map>
#include <SDL2/SDL.h>

namespace spc {

class Keyboard {
public:
    Keyboard();

    void reset();

    // 프레임 경과 틱 (키 홀드 카운터 감소 및 자동 릴리즈)
    void tick_frame();

    // 포트 주소 0x8000 ~ 0x8009 읽기
    uint8_t read_matrix(uint16_t port) const;

    // SDL 키 이벤트 처리
    void handle_key_down(SDL_Keycode sym);
    void handle_key_up(SDL_Keycode sym);

private:
    struct KeyState {
        int row{-1};
        uint8_t mask{0};
        bool is_down{false};
        int hold_ticks{0};
    };

    std::unordered_map<SDL_Keycode, KeyState> key_states_;
    std::array<uint8_t, 10> matrix_{};
};

} // namespace spc
