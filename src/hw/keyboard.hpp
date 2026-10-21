#pragma once
#include <cstdint>
#include <array>
#include <SDL2/SDL.h>

namespace spc {

class Keyboard {
public:
    Keyboard();

    void reset();

    // 포트 주소 0x8000 ~ 0x8009 읽기
    uint8_t read_matrix(uint16_t port) const;

    // SDL 키 이벤트 처리
    void handle_key_down(SDL_Keycode sym);
    void handle_key_up(SDL_Keycode sym);

private:
    void update_key(SDL_Keycode sym, bool pressed);

    // 10개 행의 키 매트릭스
    std::array<uint8_t, 10> matrix_{};
};

} // namespace spc
