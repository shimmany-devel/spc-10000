#include "vdg_mc6847.hpp"
#include "font_rom.hpp"
#include <cstring>
#include <algorithm>
#include <iostream>

namespace spc {

// Standard MC6847 Colors (RGBA32)
static const uint32_t kPalette[8] = {
    0xFF00E000, // 0: Green
    0xFF00E0E0, // 1: Yellow
    0xFFE00000, // 2: Blue
    0xFF0000E0, // 3: Red
    0xFFE0E0E0, // 4: White/Buff
    0xFFE0E000, // 5: Cyan
    0xFFE000E0, // 6: Magenta
    0xFF0080FF  // 7: Orange
};
static const uint32_t kColorBlack = 0xFF000000;

VdgMc6847::VdgMc6847() {
    reset();
}

void VdgMc6847::reset() {
    vram_.fill(0x00);
    gmode_ = 0x00;
}

uint8_t VdgMc6847::read_vram(uint16_t addr) const {
    return vram_[addr & 0x1FFF];
}

void VdgMc6847::write_vram(uint16_t addr, uint8_t val) {
    vram_[addr & 0x1FFF] = val;
}

uint8_t VdgMc6847::read_gmode() const {
    return gmode_;
}

void VdgMc6847::write_gmode(uint8_t val) {
    gmode_ = val;
}

void VdgMc6847::render(uint32_t* fb, int pitch_pixels) {
    if (gmode_ & 0x08) {
        render_graphic_mode(fb, pitch_pixels);
    } else {
        render_text_mode(fb, pitch_pixels);
    }
}

void VdgMc6847::render_text_mode(uint32_t* fb, int pitch) {
    // Text Mode: 32 columns x 16 rows
    // Page is determined by GMODE bits 4-5: (gmode_ & 0x30) >> 4
    const int page = (gmode_ & 0x30) >> 4;
    const int page_start = page * 0x200;
    const int attr_start = page_start + 0x800;

    for (int row = 0; row < 16; ++row) {
        for (int col = 0; col < 32; ++col) {
            const int offset = row * 32 + col;
            const uint8_t ch = vram_[(page_start + offset) & 0x1FFF];
            const uint8_t attr = vram_[(attr_start + offset) & 0x1FFF];

            // Colors based on attribute
            uint32_t fg = kPalette[0]; // Green by default
            uint32_t bg = kColorBlack;

            if (attr & 0x01) { // Inverted
                std::swap(fg, bg);
            }

            const uint8_t* font_data = nullptr;
            if (ch >= 96 && ch < 128) {
                // SPC-1000 소문자 PCG RAM (VRAM 0x1600 + (ch - 96) * 16)
                font_data = &vram_[0x1600 + (ch - 96) * 16];
            } else if (ch >= 128 && ch < 224) {
                // SPC-1000 사용자 정의 그래픽 PCG RAM (VRAM 0x1000 + (ch - 128) * 16)
                font_data = &vram_[0x1000 + (ch - 128) * 16];
            } else if (ch >= 32) {
                // 표준 MC6847 CGROM 및 확장 문자 ROM (ASCII 32 to 255)
                font_data = &kSpcFontRom[(ch - 32) * 12];
            }

            const int start_x = col * 8;
            const int start_y = row * 12;

            for (int dy = 0; dy < 12; ++dy) {
                const uint8_t bits = font_data ? font_data[dy] : 0x00;
                uint32_t* line = fb + (start_y + dy) * pitch + start_x;
                for (int dx = 0; dx < 8; ++dx) {
                    line[dx] = (bits & (0x80 >> dx)) ? fg : bg;
                }
            }
        }
    }
}

void VdgMc6847::render_graphic_mode(uint32_t* fb, int pitch) {
    // High-Res Graphics Mode (256x192, 1bpp, 6144 bytes)
    // 32 bytes per row, 192 rows
    const uint32_t fg = kPalette[0];
    const uint32_t bg = kColorBlack;

    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        uint32_t* line = fb + y * pitch;
        for (int byte_col = 0; byte_col < 32; ++byte_col) {
            const uint8_t b = vram_[y * 32 + byte_col];
            for (int dx = 0; dx < 8; ++dx) {
                line[byte_col * 8 + dx] = (b & (0x80 >> dx)) ? fg : bg;
            }
        }
    }
}

void VdgMc6847::save_state(std::ostream& os) {
    os.write(reinterpret_cast<const char*>(vram_.data()), vram_.size());
    os.write(reinterpret_cast<const char*>(&gmode_), sizeof(gmode_));
}

void VdgMc6847::load_state(std::istream& is) {
    is.read(reinterpret_cast<char*>(vram_.data()), vram_.size());
    is.read(reinterpret_cast<char*>(&gmode_), sizeof(gmode_));
}

} // namespace spc
