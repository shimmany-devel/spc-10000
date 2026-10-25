#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <memory>
#include <iosfwd>

namespace spc {

class PsgAy8910 {
public:
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int CLOCK_HZ = 4000000; // 4.0 MHz

    PsgAy8910();
    ~PsgAy8910();

    void reset();

    // 0x4000 포트 쓰기 (레지스터 번호 선택)
    void select_register(uint8_t reg);

    // 0x4001 포트 읽기/쓰기
    void write_data(uint8_t val);
    uint8_t read_data();

    uint8_t selected_register() const { return selected_reg_; }

    // 카세트 입력 비트 설정 (레지스터 14번 Port A로 읽힘)
    void set_cassette_input(uint8_t val) { cassette_input_ = val; }

    // CPU 사이클 경과에 따른 PSG 사운드 샘플 생성
    void step_cycles(uint32_t cycles);

    // 생성된 오디오 샘플 버퍼 읽기 및 비우기
    const std::vector<float>& sample_buffer() const { return sample_buffer_; }
    void clear_samples() { sample_buffer_.clear(); }

    void save_state(std::ostream& os);
    void load_state(std::istream& is);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

    std::array<uint8_t, 16> regs_{};
    uint8_t selected_reg_{0};
    uint8_t cassette_input_{0xFF};
    std::vector<float> sample_buffer_;
};

} // namespace spc

