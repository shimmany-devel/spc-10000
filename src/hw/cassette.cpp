#include "cassette.hpp"

namespace spc {

Cassette::Cassette() {
    reset();
}

void Cassette::reset() {
    control_ = 0;
    motor_on_ = false;
}

void Cassette::write_control(uint8_t val) {
    control_ = val;
    motor_on_ = (val & 0x01) != 0;
}

uint8_t Cassette::read_status() const {
    return control_;
}

} // namespace spc
