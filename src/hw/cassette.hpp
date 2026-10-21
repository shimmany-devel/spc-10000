#pragma once
#include <cstdint>

namespace spc {

class Cassette {
public:
    Cassette();

    void reset();

    // 0x6000 포트 쓰기 (모터 및 출력 펄스 제어)
    void write_control(uint8_t val);
    uint8_t read_status() const;

    bool is_motor_on() const { return motor_on_; }

private:
    uint8_t control_{0};
    bool motor_on_{false};
};

} // namespace spc
