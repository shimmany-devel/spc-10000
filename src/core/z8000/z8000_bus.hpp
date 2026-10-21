#pragma once
#include <cstdint>

namespace spc {

/**
 * Z8000 (Z8001 Segmented / Z8002 Non-segmented) CPU 버스 인터페이스
 * 물리 주소: Z8001의 경우 최대 23비트(8MB), Z8002의 경우 16비트(64KB)
 * 엔디안: Big-Endian (MSB first)
 */
class IZ8000Bus {
public:
    virtual ~IZ8000Bus() = default;

    // 메모리 접근 (8비트 바이트 & 16비트 워드)
    virtual uint8_t mem_read_byte(uint32_t addr) = 0;
    virtual void mem_write_byte(uint32_t addr, uint8_t val) = 0;

    virtual uint16_t mem_read_word(uint32_t addr) {
        uint16_t msb = mem_read_byte(addr);
        uint16_t lsb = mem_read_byte(addr + 1);
        return (msb << 8) | lsb; // Big-Endian
    }

    virtual void mem_write_word(uint32_t addr, uint16_t val) {
        mem_write_byte(addr, static_cast<uint8_t>(val >> 8)); // MSB first
        mem_write_byte(addr + 1, static_cast<uint8_t>(val & 0xFF));
    }

    // 표준 I/O 접근
    virtual uint8_t io_read_byte(uint16_t port) = 0;
    virtual void io_write_byte(uint16_t port, uint8_t val) = 0;

    virtual uint16_t io_read_word(uint16_t port) {
        uint16_t msb = io_read_byte(port);
        uint16_t lsb = io_read_byte(port + 1);
        return (msb << 8) | lsb;
    }

    virtual void io_write_word(uint16_t port, uint16_t val) {
        io_write_byte(port, static_cast<uint8_t>(val >> 8));
        io_write_byte(port + 1, static_cast<uint8_t>(val & 0xFF));
    }

    // 특별 I/O (Special I/O: MMU, 확장 그래픽, 프로세서간 메일박스)
    virtual uint8_t special_io_read(uint16_t port) { return 0xFF; }
    virtual void special_io_write(uint16_t port, uint8_t val) {}
};

} // namespace spc
