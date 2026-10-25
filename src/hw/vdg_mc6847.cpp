#include "vdg_mc6847.hpp"
#include "font_rom.hpp"
#include <cstring>
#include <algorithm>
#include <iostream>

namespace spc {

// MC6847 Standard Colors (RGBA32)
static const uint32_t kBlack   = 0xFF000000;
static const uint32_t kGreen   = 0xFF00E000;
static const uint32_t kYellow  = 0xFFFFFF40;
static const uint32_t kBlue    = 0xFF0000E0;
static const uint32_t kRed     = 0xFFE00000;
static const uint32_t kBuff    = 0xFFE0E0E0; // Buff/White
static const uint32_t kCyan    = 0xFF00E0E0;
static const uint32_t kMagenta = 0xFFE000E0;
static const uint32_t kOrange  = 0xFFFF8000;
static const uint32_t kDarkRed = 0xFF600000;

// Semigraphic Colors: 0:Black, 1:Green, 2:Yellow, 3:Blue, 4:Red, 5:Buff, 6:Cyan, 7:Magenta, 8:Orange
static const uint32_t kSemiColors[9] = {
    kBlack, kGreen, kYellow, kBlue, kRed, kBuff, kCyan, kMagenta, kOrange
};

// Text mode colors by attr & 0x03 [fg, bg]
static const uint32_t kTextColors[4][2] = {
    { kGreen, kBlack },    // 0x00: Normal Green on Black
    { kBlack, kGreen },    // 0x01: Inverse Black on Green
    { kOrange, kDarkRed }, // 0x02: Orange on DarkRed
    { kDarkRed, kOrange }  // 0x03: DarkRed on Orange
};

// Graphic mode palettes:
// 4-Color Mid-Res palettes (CSS=0 / CSS=1)
static const uint32_t kMidResPalette[2][4] = {
    { kGreen, kYellow, kBlue, kRed },       // CSS=0: Set 1
    { kBuff, kCyan, kMagenta, kOrange }     // CSS=1: Set 2
};

// 2-Color Hi-Res palettes (CSS=0 / CSS=1)
static const uint32_t kHiResPalette[2][2] = {
    { kBlack, kGreen },                     // CSS=0
    { kBlack, kBuff }                       // CSS=1
};

// Semigraphic Patterns (initialized once)
static uint8_t s_semiGr4[16][12];
static uint8_t s_semiGr6[64][12];
static bool s_semiInitialized = false;

static void init_semigraphic_patterns() {
    if (s_semiInitialized) return;
    s_semiInitialized = true;

    // Semigraphic 4: 16개 2x2 패턴 (12 라인: 6라인 상단, 6라인 하단)
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 12; ++j) {
            uint8_t val = 0;
            if (j < 6) {
                if (i & 0x08) val |= 0xF0;
                if (i & 0x04) val |= 0x0F;
            } else {
                if (i & 0x02) val |= 0xF0;
                if (i & 0x01) val |= 0x0F;
            }
            s_semiGr4[i][j] = val;
        }
    }

    // Semigraphic 6: 64개 2x3 패턴 (12 라인: 4라인 상단, 4라인 중단, 4라인 하단)
    for (int i = 0; i < 64; ++i) {
        for (int j = 0; j < 12; ++j) {
            uint8_t val = 0;
            if (j < 4) {
                if (i & 0x20) val |= 0xF0;
                if (i & 0x10) val |= 0x0F;
            } else if (j < 8) {
                if (i & 0x08) val |= 0xF0;
                if (i & 0x04) val |= 0x0F;
            } else {
                if (i & 0x02) val |= 0xF0;
                if (i & 0x01) val |= 0x0F;
            }
            s_semiGr6[i][j] = val;
        }
    }
}

VdgMc6847::VdgMc6847() {
    init_semigraphic_patterns();
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
            const uint8_t ascii = vram_[(page_start + offset) & 0x1FFF];
            const uint8_t attr = vram_[(attr_start + offset) & 0x1FFF];

            const uint8_t* font_data = nullptr;
            uint32_t fg = 0;
            uint32_t bg = 0;

            if (attr & 0x04) {
                // [세미그래픽 모드]: Bit 2 == 1
                bg = kSemiColors[0]; // Black
                if ((attr & 0x08) == 0) {
                    // Semigraphic 4 (16개 2x2 패턴, 색상은 ascii bits 4-6)
                    fg = kSemiColors[((ascii & 0x70) >> 4) + 1];
                    font_data = s_semiGr4[ascii & 0x0F];
                } else {
                    // Semigraphic 6 (64개 2x3 패턴, 색상은 attr bit 1 + ascii bits 6-7)
                    int col_idx = (((attr & 0x02) << 1) | ((ascii & 0xC0) >> 6)) + 1;
                    fg = kSemiColors[col_idx];
                    font_data = s_semiGr6[ascii & 0x3F];
                }
            } else {
                // [텍스트 및 사용자 정의 비트패턴(PCG) 모드]: Bit 2 == 0
                int color_mode = attr & 0x03;
                fg = kTextColors[color_mode][0];
                bg = kTextColors[color_mode][1];

                int ch = ascii;
                if (ch < 32 && !(attr & 0x08)) {
                    ch = 32;
                }
                if ((attr & 0x08) && (ch < 96)) {
                    ch += 128; // 확장 그래픽 PCG 영역으로 매핑
                }

                if (ch >= 96 && ch < 128) {
                    // SPC-1000 소문자 PCG RAM (VRAM 0x1600 + (ch - 96) * 16)
                    font_data = &vram_[0x1600 + (ch - 96) * 16];
                } else if (ch >= 128 && ch < 224) {
                    // SPC-1000 사용자 정의 그래픽 PCG RAM (VRAM 0x1000 + (ch - 128) * 16)
                    font_data = &vram_[0x1000 + (ch - 128) * 16];
                } else if (ch >= 32) {
                    // 표준 MC6847 CGROM 폰트
                    font_data = &kSpcFontRom[(ch - 32) * 12];
                }
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
    // High-Res & Mid-Res Graphics Modes
    // GMODE bit 0~2 (GM0, GM1, GM2) 및 bit 6 (CSS)
    const bool is_mid_res = ((gmode_ & 0x0E) == 0x0A || (gmode_ & 0x0E) == 0x0C);
    const int css = (gmode_ & 0x40) ? 1 : 0;

    if (is_mid_res) {
        // Mid-Res 4-Color Mode (128x192, 1바이트당 4픽셀, 폭 2도트)
        const uint32_t* pal = kMidResPalette[css];
        for (int y = 0; y < SCREEN_HEIGHT; ++y) {
            uint32_t* line = fb + y * pitch;
            for (int byte_col = 0; byte_col < 32; ++byte_col) {
                const uint8_t b = vram_[y * 32 + byte_col];
                for (int p = 0; p < 4; ++p) {
                    uint8_t c_idx = (b >> (6 - p * 2)) & 0x03;
                    uint32_t col = pal[c_idx];
                    int out_x = byte_col * 8 + p * 2;
                    line[out_x] = col;
                    line[out_x + 1] = col;
                }
            }
        }
    } else {
        // Hi-Res 2-Color Mode (256x192, 1바이트당 8픽셀)
        const uint32_t bg = kHiResPalette[css][0];
        const uint32_t fg = kHiResPalette[css][1];

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

