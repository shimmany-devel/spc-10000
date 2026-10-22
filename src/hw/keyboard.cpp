#include "keyboard.hpp"

namespace spc {

struct KeyDef {
    SDL_Keycode sym;
    int row;
    uint8_t mask;
};

static const KeyDef kKeyDefs[] = {
    // Row 0: Modifier keys & Special
    { SDLK_RSHIFT, 0, 0x02 },
    { SDLK_LSHIFT, 0, 0x02 },
    { SDLK_LCTRL,  0, 0x04 },
    { SDLK_RCTRL,  0, 0x04 },
    { SDLK_PAUSE,  0, 0x10 }, // BREAK
    { SDLK_LALT,   0, 0x40 }, // GRAPH
    { SDLK_RALT,   0, 0x40 },

    // Row 1: ~ Home Space Return c a q 1
    { SDLK_BACKQUOTE, 1, 0x01 }, // ~
    { SDLK_HOME,      1, 0x02 },
    { SDLK_SPACE,     1, 0x04 },
    { SDLK_RETURN,    1, 0x08 },
    { SDLK_KP_ENTER,  1, 0x08 },
    { SDLK_c,         1, 0x10 },
    { SDLK_a,         1, 0x20 },
    { SDLK_q,         1, 0x40 },
    { SDLK_1,         1, 0x80 },
    { SDLK_KP_1,      1, 0x80 },

    // Row 2: CAPS z ] v s w 2
    { SDLK_TAB,          2, 0x01 }, // CAPS
    { SDLK_CAPSLOCK,     2, 0x01 },
    { SDLK_z,            2, 0x04 },
    { SDLK_RIGHTBRACKET, 2, 0x08 }, // ]
    { SDLK_v,            2, 0x10 },
    { SDLK_s,            2, 0x20 },
    { SDLK_w,            2, 0x40 },
    { SDLK_2,            2, 0x80 },
    { SDLK_KP_2,         2, 0x80 },

    // Row 3: DEL ESC [ b d e 3
    { SDLK_BACKSPACE,   3, 0x01 }, // DEL
    { SDLK_DELETE,      3, 0x01 },
    { SDLK_ESCAPE,      3, 0x04 }, // ESC (SPC-1000 실기 사양)
    { SDLK_LEFTBRACKET, 3, 0x08 }, // [
    { SDLK_b,           3, 0x10 },
    { SDLK_d,           3, 0x20 },
    { SDLK_e,           3, 0x40 },
    { SDLK_3,           3, 0x80 },
    { SDLK_KP_3,        3, 0x80 },

    // Row 4: -> \ n f r 4
    { SDLK_RIGHT,     4, 0x04 },
    { SDLK_BACKSLASH, 4, 0x08 },
    { SDLK_n,         4, 0x10 },
    { SDLK_f,         4, 0x20 },
    { SDLK_r,         4, 0x40 },
    { SDLK_4,         4, 0x80 },
    { SDLK_KP_4,      4, 0x80 },

    // Row 5: F1 <- m g t 5
    { SDLK_F1,   5, 0x02 },
    { SDLK_LEFT, 5, 0x04 },
    { SDLK_m,    5, 0x10 },
    { SDLK_g,    5, 0x20 },
    { SDLK_t,    5, 0x40 },
    { SDLK_5,    5, 0x80 },
    { SDLK_KP_5, 5, 0x80 },

    // Row 6: F2 @ x , h y 6
    { SDLK_F2,     6, 0x02 },
    { SDLK_EQUALS, 6, 0x04 }, // @ / =
    { SDLK_x,      6, 0x08 },
    { SDLK_COMMA,  6, 0x10 },
    { SDLK_h,      6, 0x20 },
    { SDLK_y,      6, 0x40 },
    { SDLK_6,      6, 0x80 },
    { SDLK_KP_6,   6, 0x80 },

    // Row 7: F3 ^ p . j u 7
    { SDLK_F3,        7, 0x02 },
    { SDLK_UP,        7, 0x04 },
    { SDLK_p,         7, 0x08 },
    { SDLK_PERIOD,    7, 0x10 },
    { SDLK_KP_PERIOD, 7, 0x10 },
    { SDLK_j,         7, 0x20 },
    { SDLK_u,         7, 0x40 },
    { SDLK_7,         7, 0x80 },
    { SDLK_KP_7,      7, 0x80 },

    // Row 8: F4 v : / k i 8
    { SDLK_F4,        8, 0x02 },
    { SDLK_DOWN,      8, 0x04 },
    { SDLK_QUOTE,     8, 0x08 }, // :
    { SDLK_SLASH,     8, 0x10 },
    { SDLK_KP_DIVIDE, 8, 0x10 },
    { SDLK_k,         8, 0x20 },
    { SDLK_i,         8, 0x40 },
    { SDLK_8,         8, 0x80 },
    { SDLK_KP_8,      8, 0x80 },

    // Row 9: F5 - 0 ; l o 9
    { SDLK_F5,        9, 0x02 },
    { SDLK_MINUS,     9, 0x04 },
    { SDLK_KP_MINUS,  9, 0x04 },
    { SDLK_0,         9, 0x08 },
    { SDLK_KP_0,      9, 0x08 },
    { SDLK_SEMICOLON, 9, 0x10 },
    { SDLK_l,         9, 0x20 },
    { SDLK_o,         9, 0x40 },
    { SDLK_9,         9, 0x80 },
    { SDLK_KP_9,      9, 0x80 }
};

Keyboard::Keyboard() {
    reset();
}

void Keyboard::reset() {
    matrix_.fill(0xFF);
    key_states_.clear();
}

uint8_t Keyboard::read_matrix(uint16_t port) const {
    const size_t row = port - 0x8000;
    if (row < matrix_.size()) {
        return matrix_[row];
    }
    return 0xFF;
}

void Keyboard::handle_key_down(SDL_Keycode sym) {
    for (const auto& def : kKeyDefs) {
        if (def.sym == sym) {
            auto& state = key_states_[sym];
            state.row = def.row;
            state.mask = def.mask;
            state.is_down = true;

            // Shift, Ctrl, Alt, Caps, 방향키는 물리적 누름 동안만 유지
            bool is_modifier_or_arrow = (def.row == 0) || (def.row == 2 && def.mask == 0x01) ||
                (sym == SDLK_UP || sym == SDLK_DOWN || sym == SDLK_LEFT || sym == SDLK_RIGHT);

            // 일반 문자/숫자 키는 최소 3프레임(약 50ms) 동안 신호를 강제 유지하여
            // Hu-BASIC 키보드 디바운스 루틴이 100% 감지하도록 보장
            state.hold_ticks = is_modifier_or_arrow ? 0 : 3;

            matrix_[def.row] &= ~def.mask; // Active-low: 0
        }
    }
}

void Keyboard::handle_key_up(SDL_Keycode sym) {
    auto it = key_states_.find(sym);
    if (it != key_states_.end()) {
        auto& state = it->second;
        state.is_down = false;

        // 최소 유지 시간이 지났으면 즉시 릴리즈
        if (state.hold_ticks <= 0) {
            matrix_[state.row] |= state.mask; // Active-low 릴리즈: 1
        }
        // hold_ticks > 0인 경우 tick_frame()에서 0이 될 때까지 대기
    }
}

void Keyboard::tick_frame() {
    for (auto& [sym, state] : key_states_) {
        if (state.hold_ticks > 0) {
            state.hold_ticks--;
            if (state.hold_ticks == 0 && !state.is_down) {
                matrix_[state.row] |= state.mask;
            }
        }
    }
}

} // namespace spc
