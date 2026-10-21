#pragma once
#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include "../core/z8000/z8000_bus.hpp"
#include "../hw/vdg_mc6847.hpp"
#include "../hw/psg_ay8910.hpp"
#include "../hw/keyboard.hpp"
#include "../hw/cassette.hpp"

namespace spc {

class Spc10000Board : public IZ8000Bus {
public:
    static constexpr size_t SEGMENT_SIZE = 0x10000; // 64KB
    static constexpr size_t NUM_SEGMENTS = 4;       // 세그먼트 0~3 (총 256KB 기본 확장)
    static constexpr size_t ROM_SIZE = 0x8000;      // 32KB Native ROM

    Spc10000Board();

    bool load_native_rom(const std::string& filepath);
    void reset();

    // IZ8000Bus 메모리 접근 구현
    uint8_t mem_read_byte(uint32_t addr) override;
    void mem_write_byte(uint32_t addr, uint8_t val) override;
    uint16_t mem_read_word(uint32_t addr) override;
    void mem_write_word(uint32_t addr, uint16_t val) override;

    // IZ8000Bus I/O 버스 브릿징 구현
    uint8_t io_read_byte(uint16_t port) override;
    void io_write_byte(uint16_t port, uint8_t val) override;
    uint16_t io_read_word(uint16_t port) override;
    void io_write_word(uint16_t port, uint16_t val) override;

    // Special I/O
    uint8_t special_io_read(uint16_t port) override;
    void special_io_write(uint16_t port, uint8_t val) override;

    // 하드웨어 모듈 접근자
    VdgMc6847& vdg() { return vdg_; }
    Keyboard& keyboard() { return keyboard_; }
    PsgAy8910& psg() { return psg_; }
    Cassette& cassette() { return cassette_; }

    // 확장 슈퍼 그래픽 모드 (256컬러 프레임버퍼) 활성 여부
    bool is_super_graphic_mode() const { return super_graphic_mode_; }
    void render_super_graphic(uint32_t* fb, int pitch);

private:
    std::array<uint8_t, ROM_SIZE> rom_{}; // 세그먼트 0의 0x0000 - 0x7FFF
    std::vector<uint8_t> ram_;            // 확장 세그먼트 메모리 (256KB)
    bool rom_active_{true};

    // 확장 제어 상태
    bool super_graphic_mode_{false};
    uint8_t mailbox_sub_to_main_{0};
    uint8_t mailbox_main_to_sub_{0};

    // 하드웨어 디바이스
    VdgMc6847 vdg_;
    Keyboard keyboard_;
    PsgAy8910 psg_;
    Cassette cassette_;
};

} // namespace spc
