#pragma once
#include <cstdint>
#include <memory>
#include <iosfwd>
#include "bus_interface.hpp"

namespace spc {

class Z80Cpu {
public:
    explicit Z80Cpu(IBus& bus);
    ~Z80Cpu();

    // CPU 리셋
    void reset();

    // 특정 PC로 즉시 강제 점프 (예: $004A 부트 진입점)
    void prefetch(uint16_t pc);

    // 지정된 사이클 수만큼 CPU 실행 (반환값: 실제 소비된 사이클)
    uint32_t step_cycles(uint32_t target_cycles);

    // 1개 인스트럭션 완료될 때까지 실행 (반환값: 소비된 사이클)
    uint32_t step_instruction();

    // 인터럽트 요청 (VBLANK 등)
    void trigger_irq();

    uint16_t pc() const;
    uint16_t sp() const;

    void save_state(std::ostream& os);
    void load_state(std::istream& is);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace spc
