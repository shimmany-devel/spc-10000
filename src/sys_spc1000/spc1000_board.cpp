#include "spc1000_board.hpp"
#include <fstream>
#include <iostream>

namespace spc {

Spc1000Board::Spc1000Board() {
    reset();
}

void Spc1000Board::reset() {
    ram_.fill(0x00);
    iplk_ = true; // ROM active on reset
    vdg_.reset();
    keyboard_.reset();
    psg_.reset();
    cassette_.reset();
}

bool Spc1000Board::load_rom(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[Spc1000Board] Failed to open ROM: " << filepath << "\n";
        return false;
    }

    file.read(reinterpret_cast<char*>(rom_.data()), ROM_SIZE);
    std::streamsize bytes_read = file.gcount();
    std::cout << "[Spc1000Board] Loaded " << bytes_read << " bytes from " << filepath << "\n";

    return bytes_read > 0;
}

uint8_t Spc1000Board::mem_read(uint16_t addr) {
    if (iplk_) {
        return rom_[addr & 0x7FFF];
    }
    return ram_[addr];
}

void Spc1000Board::mem_write(uint16_t addr, uint8_t val) {
    // 섀도우 RAM을 포함하여 쓰기는 항상 RAM에 반영됨
    ram_[addr] = val;
}

uint8_t Spc1000Board::io_read(uint16_t port) {
    // 1. 키보드 매트릭스 (0x8000 ~ 0x8009)
    if (port >= 0x8000 && port <= 0x8009) {
        return keyboard_.read_matrix(port);
    }

    // 2. IPLK 뱅크 스위칭 토글 (0xA000 ~ 0xBFFF)
    if ((port & 0xE000) == 0xA000) {
        iplk_ = !iplk_;
        return 0xFF;
    }

    // 3. VRAM 읽기 (0x0000 ~ 0x1FFF)
    if ((port & 0xE000) == 0x0000) {
        return vdg_.read_vram(port);
    }

    // 4. VDG GMODE 읽기 (0x2000 ~ 0x3FFF)
    if ((port & 0xE000) == 0x2000) {
        return vdg_.read_gmode();
    }

    // 5. PSG 사운드 데이터 읽기 (0x4001)
    if ((port & 0xFFFE) == 0x4000) {
        if (port & 0x01) {
            return psg_.read_data();
        }
    }

    return 0xFF;
}

void Spc1000Board::io_write(uint16_t port, uint8_t val) {
    // 1. VRAM 쓰기 (0x0000 ~ 0x1FFF)
    if ((port & 0xE000) == 0x0000) {
        vdg_.write_vram(port, val);
        return;
    }

    // 2. IPLK 뱅크 토글 (0xA000 ~ 0xBFFF)
    if ((port & 0xE000) == 0xA000) {
        iplk_ = !iplk_;
        return;
    }

    // 3. VDG GMODE 쓰기 (0x2000 ~ 0x3FFF)
    if ((port & 0xE000) == 0x2000) {
        vdg_.write_gmode(val);
        return;
    }

    // 4. PSG 쓰기 (0x4000: 레지스터 번호, 0x4001: 데이터)
    if ((port & 0xFFFE) == 0x4000) {
        if (port & 0x01) {
            psg_.write_data(val);
        } else {
            psg_.select_register(val);
        }
        return;
    }

    // 5. 카세트 제어 쓰기 (0x6000 ~ 0x7FFF)
    if ((port & 0xE000) == 0x6000) {
        cassette_.write_control(val);
        return;
    }
}

} // namespace spc
