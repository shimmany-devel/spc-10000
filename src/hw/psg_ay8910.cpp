#include "psg_ay8910.hpp"

namespace spc {

PsgAy8910::PsgAy8910() {
    reset();
}

void PsgAy8910::reset() {
    regs_.fill(0x00);
    selected_reg_ = 0;
    cassette_input_ = 0xFF;
}

void PsgAy8910::select_register(uint8_t reg) {
    selected_reg_ = reg & 0x0F;
}

void PsgAy8910::write_data(uint8_t val) {
    if (selected_reg_ < regs_.size()) {
        regs_[selected_reg_] = val;
    }
}

uint8_t PsgAy8910::read_data() {
    if (selected_reg_ == 14) {
        // Port A reads cassette signal
        return cassette_input_;
    }
    if (selected_reg_ < regs_.size()) {
        return regs_[selected_reg_];
    }
    return 0xFF;
}

} // namespace spc
