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

    if (bytes_read > 0) {
        // 현대 PC 키보드 자판 레이아웃 호환 패치 (:, ", +, -, @ 등 특수기호 매핑 보정)
        rom_[0x1311] = 0x27; // '
        rom_[0x1320] = 0x3d; // =
        rom_[0x1331] = 0x7c; // |
        rom_[0x1341] = 0x5d; // ]
        rom_[0x1346] = 0x60; // `
        rom_[0x1350] = 0x7f;
        rom_[0x1351] = 0x29; // )
        rom_[0x1352] = 0x3a; // :
        rom_[0x1355] = 0x28; // (
        rom_[0x1359] = 0x22; // "
        rom_[0x135d] = 0x2a; // *
        rom_[0x1365] = 0x26; // &
        rom_[0x1368] = 0x2b; // +
        rom_[0x136d] = 0x5e; // ^
        rom_[0x1379] = 0x5c; // backslash
        rom_[0x138d] = 0x40; // @
        rom_[0x138e] = 0x7e; // ~
        return true;
    }
    return false;
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
            if (psg_.selected_register() == 14) {
                return cassette_.read_io();
            }
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

void Spc1000Board::save_state(std::ostream& os) {
    os.write(reinterpret_cast<const char*>(ram_.data()), ram_.size());
    os.write(reinterpret_cast<const char*>(&iplk_), sizeof(iplk_));
    vdg_.save_state(os);
    psg_.save_state(os);
    cassette_.save_state(os);
}

void Spc1000Board::load_state(std::istream& is) {
    is.read(reinterpret_cast<char*>(ram_.data()), ram_.size());
    is.read(reinterpret_cast<char*>(&iplk_), sizeof(iplk_));
    vdg_.load_state(is);
    psg_.load_state(is);
    cassette_.load_state(is);
}

} // namespace spc
