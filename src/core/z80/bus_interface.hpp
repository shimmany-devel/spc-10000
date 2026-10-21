#pragma once
#include <cstdint>

namespace spc {

/**
 * Z80 버스 상호작용을 위한 순수 가상 인터페이스
 */
class IBus {
public:
    virtual ~IBus() = default;

    // 메모리 읽기/쓰기
    virtual uint8_t mem_read(uint16_t addr) = 0;
    virtual void mem_write(uint16_t addr, uint8_t val) = 0;

    // I/O 포트 읽기/쓰기
    virtual uint8_t io_read(uint16_t port) = 0;
    virtual void io_write(uint16_t port, uint8_t val) = 0;
};

} // namespace spc
