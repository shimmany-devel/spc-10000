#include "spc10000_board.hpp"
#include <fstream>
#include <iostream>

namespace spc {

Spc10000Board::Spc10000Board() {
    ram_.resize(NUM_SEGMENTS * SEGMENT_SIZE, 0x00);
    reset();
}

void Spc10000Board::reset() {
    std::fill(ram_.begin(), ram_.end(), 0x00);
    rom_active_ = true;
    super_graphic_mode_ = false;
    mailbox_sub_to_main_ = 0;
    mailbox_main_to_sub_ = 0;

    vdg_.reset();
    keyboard_.reset();
    psg_.reset();
    cassette_.reset();
}

bool Spc10000Board::load_native_rom(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[Spc10000Board] Native ROM not found: " << filepath << "\n";
        return false;
    }

    file.read(reinterpret_cast<char*>(rom_.data()), ROM_SIZE);
    std::streamsize bytes_read = file.gcount();
    std::cout << "[Spc10000Board] Loaded " << bytes_read << " bytes into Segment 0 ROM.\n";
    return bytes_read > 0;
}

uint8_t Spc10000Board::mem_read_byte(uint32_t addr) {
    // 23비트 물리 주소 마스크 (8MB)
    addr &= 0x7FFFFF;

    // Segment 0 (0x000000 - 0x007FFF) ROM 영역
    if (rom_active_ && addr < ROM_SIZE) {
        return rom_[addr];
    }

    if (addr < ram_.size()) {
        return ram_[addr];
    }
    return 0xFF;
}

void Spc10000Board::mem_write_byte(uint32_t addr, uint8_t val) {
    addr &= 0x7FFFFF;
    if (addr < ram_.size()) {
        ram_[addr] = val;
    }
}

uint16_t Spc10000Board::mem_read_word(uint32_t addr) {
    // Big-Endian: MSB at lower address
    uint16_t msb = mem_read_byte(addr);
    uint16_t lsb = mem_read_byte(addr + 1);
    return (msb << 8) | lsb;
}

void Spc10000Board::mem_write_word(uint32_t addr, uint16_t val) {
    mem_write_byte(addr, static_cast<uint8_t>(val >> 8));
    mem_write_byte(addr + 1, static_cast<uint8_t>(val & 0xFF));
}

uint8_t Spc10000Board::io_read_byte(uint16_t port) {
    // 1. 키보드 매트릭스 (0x8000 ~ 0x8009)
    if (port >= 0x8000 && port <= 0x8009) {
        return keyboard_.read_matrix(port);
    }

    // 2. VRAM 윈도우 읽기 (0x0000 ~ 0x1FFF)
    if ((port & 0xE000) == 0x0000) {
        return vdg_.read_vram(port);
    }

    // 3. VDG GMODE (0x2000 ~ 0x3FFF)
    if ((port & 0xE000) == 0x2000) {
        return vdg_.read_gmode();
    }

    // 4. PSG 사운드 데이터 (0x4001)
    if ((port & 0xFFFE) == 0x4000) {
        if (port & 0x01) {
            return psg_.read_data();
        }
    }

    return 0xFF;
}

void Spc10000Board::io_write_byte(uint16_t port, uint8_t val) {
    // 1. VRAM 윈도우 쓰기 (0x0000 ~ 0x1FFF)
    if ((port & 0xE000) == 0x0000) {
        vdg_.write_vram(port, val);
        return;
    }

    // 2. VDG GMODE 쓰기 (0x2000 ~ 0x3FFF)
    if ((port & 0xE000) == 0x2000) {
        vdg_.write_gmode(val);
        return;
    }

    // 3. PSG 사운드 쓰기 (0x4000: reg, 0x4001: data)
    if ((port & 0xFFFE) == 0x4000) {
        if (port & 0x01) {
            psg_.write_data(val);
        } else {
            psg_.select_register(val);
        }
        return;
    }

    // 4. 카세트 제어 쓰기 (0x6000 ~ 0x7FFF)
    if ((port & 0xE000) == 0x6000) {
        cassette_.write_control(val);
        return;
    }
}

uint16_t Spc10000Board::io_read_word(uint16_t port) {
    // 16비트 포트 I/O: 상위 바이트와 하위 바이트 결합
    uint16_t msb = io_read_byte(port);
    uint16_t lsb = io_read_byte(port + 1);
    return (msb << 8) | lsb;
}

void Spc10000Board::io_write_word(uint16_t port, uint16_t val) {
    io_write_byte(port, static_cast<uint8_t>(val >> 8));
    io_write_byte(port + 1, static_cast<uint8_t>(val & 0xFF));
}

uint8_t Spc10000Board::special_io_read(uint16_t port) {
    switch (port & 0xFF) {
        case 0x00: // 확장 슈퍼 그래픽 모드 상태
            return super_graphic_mode_ ? 0x01 : 0x00;
        case 0x01: // 서브 CPU 메일박스 읽기
            return mailbox_sub_to_main_;
        case 0x02: // ROM 활성 상태
            return rom_active_ ? 0x01 : 0x00;
        default:
            return 0xFF;
    }
}

void Spc10000Board::special_io_write(uint16_t port, uint8_t val) {
    switch (port & 0xFF) {
        case 0x00: // 확장 슈퍼 그래픽 모드 온/오프
            super_graphic_mode_ = (val & 0x01) != 0;
            std::cout << "[Spc10000Board] Super Graphic Mode: " << (super_graphic_mode_ ? "ON" : "OFF") << "\n";
            break;
        case 0x01: // 서브 CPU로 메시지 전송
            mailbox_main_to_sub_ = val;
            break;
        case 0x02: // ROM 뱅크 비활성화 (All RAM 모드 진입)
            rom_active_ = (val & 0x01) != 0;
            break;
    }
}

void Spc10000Board::render_super_graphic(uint32_t* fb, int pitch) {
    // Segment 2 (0x020000 ~ 0x02BFFF) 48,000 바이트 = 256x192 픽셀
    // 각 바이트는 RGB332 색상 형식 (RRR GGG BB)
    constexpr uint32_t seg2_base = 2 * SEGMENT_SIZE;
    for (int y = 0; y < VdgMc6847::SCREEN_HEIGHT; ++y) {
        uint32_t* line = fb + y * pitch;
        for (int x = 0; x < VdgMc6847::SCREEN_WIDTH; ++x) {
            uint32_t addr = seg2_base + y * VdgMc6847::SCREEN_WIDTH + x;
            uint8_t color_byte = (addr < ram_.size()) ? ram_[addr] : 0x00;

            // RGB332 -> ARGB8888 변환
            uint8_t r = ((color_byte >> 5) & 0x07) * 36;
            uint8_t g = ((color_byte >> 2) & 0x07) * 36;
            uint8_t b = (color_byte & 0x03) * 85;

            line[x] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }
}

} // namespace spc
