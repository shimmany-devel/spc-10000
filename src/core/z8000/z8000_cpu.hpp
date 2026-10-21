#pragma once
#include <memory>
#include "z8000_bus.hpp"
#include "z8000_types.hpp"

namespace spc {

class Z8000Cpu {
public:
    enum class Model {
        Z8001_Segmented,
        Z8002_NonSegmented
    };

    explicit Z8000Cpu(IZ8000Bus& bus, Model model = Model::Z8001_Segmented);
    ~Z8000Cpu();

    // CPU 리셋 (Program Status Area의 0번지 벡터 로드)
    void reset();

    // 특정 주소로 강제 시작 위치 설정
    void set_pc(uint8_t seg, uint16_t off);
    void set_pc_linear(uint32_t addr);

    // 1개 명령어 실행 (반환값: 소비된 클럭 사이클)
    uint32_t step_instruction();

    // 지정된 사이클만큼 실행
    uint32_t step_cycles(uint32_t target_cycles);

    // 인터럽트 발생
    void trigger_nmi();
    void trigger_vi(uint16_t vector);
    void trigger_nvi();

    // 레지스터 및 상태 접근
    const Z8000Registers& regs() const;
    Z8000Registers& regs();
    Model model() const { return model_; }
    bool is_segmented() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
    Model model_;
};

} // namespace spc
