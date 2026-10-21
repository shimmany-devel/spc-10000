#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include "../core/z80/bus_interface.hpp"
#include "../hw/vdg_mc6847.hpp"
#include "../hw/psg_ay8910.hpp"
#include "../hw/keyboard.hpp"
#include "../hw/cassette.hpp"

namespace spc {

class Spc1000Board : public IBus {
public:
    static constexpr size_t RAM_SIZE = 0x10000; // 64KB
    static constexpr size_t ROM_SIZE = 0x8000;  // 32KB

    Spc1000Board();

    bool load_rom(const std::string& filepath);
    void reset();

    // IBus 구현
    uint8_t mem_read(uint16_t addr) override;
    void mem_write(uint16_t addr, uint8_t val) override;
    uint8_t io_read(uint16_t port) override;
    void io_write(uint16_t port, uint8_t val) override;

    // 하드웨어 모듈 접근자
    VdgMc6847& vdg() { return vdg_; }
    Keyboard& keyboard() { return keyboard_; }
    PsgAy8910& psg() { return psg_; }
    Cassette& cassette() { return cassette_; }

    bool iplk() const { return iplk_; }
    void set_iplk(bool active) { iplk_ = active; }

    uint8_t* ram_data() { return ram_.data(); }
    uint8_t* rom_data() { return rom_.data(); }

private:
    std::array<uint8_t, RAM_SIZE> ram_{};
    std::array<uint8_t, ROM_SIZE> rom_{};
    bool iplk_{true}; // 부팅 시 기본 true (ROM 모드)

    VdgMc6847 vdg_;
    Keyboard keyboard_;
    PsgAy8910 psg_;
    Cassette cassette_;
};

} // namespace spc
