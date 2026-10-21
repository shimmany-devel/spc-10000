#include "keyboard.hpp"

namespace spc {

struct KeyEntry {
    SDL_Keycode sym;
    int row;
    uint8_t mask;
};

static const KeyEntry kKeyTable[] = {
    // Row 0
    { SDLK_RSHIFT, 0, 0x02 },
    { SDLK_LSHIFT, 0, 0x02 },
    { SDLK_LCTRL,  0, 0x04 },
    { SDLK_RCTRL,  0, 0x04 },
    { SDLK_PAUSE,  0, 0x10 }, // BREAK
    { SDLK_ESCAPE, 0, 0x10 }, // Alt BREAK
    { SDLK_LALT,   0, 0x40 }, // GRAPH
    { SDLK_RALT,   0, 0x40 },

    // Row 1
    { SDLK_BACKQUOTE, 1, 0x01 }, // ~
    { SDLK_HOME,      1, 0x02 },
    { SDLK_SPACE,     1, 0x04 },
    { SDLK_RETURN,    1, 0x08 },
    { SDLK_c,         1, 0x10 },
    { SDLK_a,         1, 0x20 },
    { SDLK_q,         1, 0x40 },
    { SDLK_1,         1, 0x80 },

    // Row 2
    { SDLK_TAB,          2, 0x01 }, // CAPS
    { SDLK_z,            2, 0x04 },
    { SDLK_RIGHTBRACKET, 2, 0x08 }, // ]
    { SDLK_v,            2, 0x10 },
    { SDLK_s,            2, 0x20 },
    { SDLK_w,            2, 0x40 },
    { SDLK_2,            2, 0x80 },

    // Row 3
    { SDLK_BACKSPACE,   3, 0x01 }, // DEL
    { SDLK_LEFTBRACKET, 3, 0x08 }, // [
    { SDLK_b,           3, 0x10 },
    { SDLK_d,           3, 0x20 },
    { SDLK_e,           3, 0x40 },
    { SDLK_3,           3, 0x80 },

    // Row 4
    { SDLK_RIGHT,     4, 0x04 },
    { SDLK_BACKSLASH, 4, 0x08 },
    { SDLK_n,         4, 0x10 },
    { SDLK_f,         4, 0x20 },
    { SDLK_r,         4, 0x40 },
    { SDLK_4,         4, 0x80 },

    // Row 5
    { SDLK_F1,   5, 0x02 },
    { SDLK_LEFT, 5, 0x04 },
    { SDLK_m,    5, 0x10 },
    { SDLK_g,    5, 0x20 },
    { SDLK_t,    5, 0x40 },
    { SDLK_5,    5, 0x80 },

    // Row 6
    { SDLK_F2,     6, 0x02 },
    { SDLK_EQUALS, 6, 0x04 }, // @
    { SDLK_x,      6, 0x08 },
    { SDLK_COMMA,  6, 0x10 },
    { SDLK_h,      6, 0x20 },
    { SDLK_y,      6, 0x40 },
    { SDLK_6,      6, 0x80 },

    // Row 7
    { SDLK_F3,     7, 0x02 },
    { SDLK_UP,     7, 0x04 },
    { SDLK_p,      7, 0x08 },
    { SDLK_PERIOD, 7, 0x10 },
    { SDLK_j,      7, 0x20 },
    { SDLK_u,      7, 0x40 },
    { SDLK_7,      7, 0x80 },

    // Row 8
    { SDLK_F4,    8, 0x02 },
    { SDLK_DOWN,  8, 0x04 },
    { SDLK_QUOTE, 8, 0x08 }, // :
    { SDLK_SLASH, 8, 0x10 },
    { SDLK_k,     8, 0x20 },
    { SDLK_i,     8, 0x40 },
    { SDLK_8,     8, 0x80 },

    // Row 9
    { SDLK_F5,        9, 0x02 },
    { SDLK_MINUS,     9, 0x04 },
    { SDLK_0,         9, 0x08 },
    { SDLK_SEMICOLON, 9, 0x10 },
    { SDLK_l,         9, 0x20 },
    { SDLK_o,         9, 0x40 },
    { SDLK_9,         9, 0x80 }
};

Keyboard::Keyboard() {
    reset();
}

void Keyboard::reset() {
    matrix_.fill(0xFF);
}

uint8_t Keyboard::read_matrix(uint16_t port) const {
    const size_t row = port - 0x8000;
    if (row < matrix_.size()) {
        return matrix_[row];
    }
    return 0xFF;
}

void Keyboard::update_key(SDL_Keycode sym, bool pressed) {
    for (const auto& entry : kKeyTable) {
        if (entry.sym == sym) {
            if (pressed) {
                matrix_[entry.row] &= ~entry.mask; // Active-low
            } else {
                matrix_[entry.row] |= entry.mask;
            }
        }
    }
}

void Keyboard::handle_key_down(SDL_Keycode sym) {
    update_key(sym, true);
}

void Keyboard::handle_key_up(SDL_Keycode sym) {
    update_key(sym, false);
}

} // namespace spc
