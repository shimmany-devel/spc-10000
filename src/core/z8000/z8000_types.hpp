#pragma once
#include <cstdint>
#include <array>

namespace spc {

// Flag Control Word (FCW) 비트 정의
enum Z8000FcwBits : uint16_t {
    FCW_SEG   = 0x8000, // Segmented Mode (Z8001 전용)
    FCW_S_N   = 0x4000, // 1: System Mode, 0: Normal Mode
    FCW_EPU   = 0x2000, // Extension Processor Unit Trap
    FCW_VIE   = 0x1000, // Vectored Interrupt Enable
    FCW_NVIE  = 0x0800, // Non-Vectored Interrupt Enable
    FCW_C     = 0x0080, // Carry Flag
    FCW_Z     = 0x0040, // Zero Flag
    FCW_S     = 0x0020, // Sign Flag
    FCW_PV    = 0x0010, // Parity / Overflow Flag
    FCW_DA    = 0x0008, // Decimal Adjust Flag
    FCW_H     = 0x0004  // Half Carry Flag
};

// Z8000 레지스터 파일
struct Z8000Registers {
    // 16개의 16비트 범용 레지스터
    std::array<uint16_t, 16> r{};

    // 시스템 모드 / 노멀 모드 스택 포인터 분리 보관
    uint16_t r15_sys{0};
    uint16_t r15_norm{0};
    uint16_t r14_sys{0};
    uint16_t r14_norm{0};

    // 플래그 및 컨트롤 워드 (FCW)
    uint16_t fcw{FCW_S_N}; // 부팅 시 System Mode로 초기화

    // 프로그램 카운터 (Z8001: seg(7비트) + offset(16비트), Z8002: 16비트)
    uint8_t pc_seg{0};
    uint16_t pc_off{0};

    // 프로그램 상태 영역 포인터 (Program Status Area Pointer)
    uint8_t psap_seg{0};
    uint16_t psap_off{0};

    // 8비트 바이트 레지스터 접근자 (RH0~RH7, RL0~RL7)
    uint8_t get_rb(int reg_idx) const {
        int r_num = (reg_idx < 8) ? (reg_idx / 2) : ((reg_idx - 8) / 2 + 4);
        bool is_high = (reg_idx % 2 == 0);
        return is_high ? static_cast<uint8_t>(r[r_num] >> 8) : static_cast<uint8_t>(r[r_num] & 0xFF);
    }

    void set_rb(int reg_idx, uint8_t val) {
        int r_num = (reg_idx < 8) ? (reg_idx / 2) : ((reg_idx - 8) / 2 + 4);
        bool is_high = (reg_idx % 2 == 0);
        if (is_high) {
            r[r_num] = (r[r_num] & 0x00FF) | (static_cast<uint16_t>(val) << 8);
        } else {
            r[r_num] = (r[r_num] & 0xFF00) | val;
        }
    }

    // 32비트 레지스터 페어 접근자 (RR0, RR2, ..., RR14)
    uint32_t get_rr(int reg_idx) const {
        return (static_cast<uint32_t>(r[reg_idx]) << 16) | r[reg_idx + 1];
    }

    void set_rr(int reg_idx, uint32_t val) {
        r[reg_idx] = static_cast<uint16_t>(val >> 16);
        r[reg_idx + 1] = static_cast<uint16_t>(val & 0xFFFF);
    }

    // 현재 23비트 물리 주소 계산 (Z8001)
    uint32_t physical_pc() const {
        if (fcw & FCW_SEG) {
            return (static_cast<uint32_t>(pc_seg & 0x7F) << 16) | pc_off;
        }
        return pc_off;
    }
};

} // namespace spc
