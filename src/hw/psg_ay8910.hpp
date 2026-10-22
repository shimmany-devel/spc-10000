#pragma once
#include <cstdint>
#include <array>
#include <iosfwd>

namespace spc {

class PsgAy8910 {
public:
    PsgAy8910();

    void reset();

    // 0x4000 포트 쓰기 (레지스터 번호 선택)
    void select_register(uint8_t reg);

    // 0x4001 포트 읽기/쓰기
    void write_data(uint8_t val);
    uint8_t read_data();

    uint8_t selected_register() const { return selected_reg_; }

    // 카세트 입력 비트 설정 (레지스터 14번 Port A로 읽힘)
    void set_cassette_input(uint8_t val) { cassette_input_ = val; }

    void save_state(std::ostream& os);
    void load_state(std::istream& is);

private:
    std::array<uint8_t, 16> regs_{};
    uint8_t selected_reg_{0};
    uint8_t cassette_input_{0xFF};
};

} // namespace spc
