#include "psg_ay8910.hpp"
#include <iostream>

#define CHIPS_IMPL
#include "../../reference/chips-repo/chips/ay38910.h"

namespace spc {

struct PsgAy8910::Impl {
    ay38910_t ay{};

    Impl() {
        ay38910_desc_t desc{};
        desc.type = AY38910_TYPE_8910;
        desc.tick_hz = CLOCK_HZ;
        desc.sound_hz = SAMPLE_RATE;
        desc.magnitude = 0.25f; // 3채널 합산 시 왜곡 방지 적정 진폭
        ay38910_init(&ay, &desc);
    }

    void reset() {
        ay38910_reset(&ay);
    }
};

PsgAy8910::PsgAy8910() : pimpl_(std::make_unique<Impl>()) {
    reset();
}

PsgAy8910::~PsgAy8910() = default;

void PsgAy8910::reset() {
    regs_.fill(0x00);
    selected_reg_ = 0;
    cassette_input_ = 0xFF;
    cycle_accumulator_ = 0;
    sample_buffer_.clear();
    sample_buffer_.reserve(2048);
    if (pimpl_) {
        pimpl_->reset();
    }
}

void PsgAy8910::select_register(uint8_t reg) {
    selected_reg_ = reg & 0x0F;
    if (pimpl_) {
        ay38910_set_addr_latch(&pimpl_->ay, selected_reg_);
    }
}

void PsgAy8910::write_data(uint8_t val) {
    if (selected_reg_ < regs_.size()) {
        regs_[selected_reg_] = val;
    }
    if (pimpl_) {
        ay38910_set_register(&pimpl_->ay, selected_reg_, val);
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

void PsgAy8910::step_cycles(uint32_t cycles) {
    if (!pimpl_) return;
    // Z80 CPU(4.0MHz) 사이클을 AY-3-8910 PSG(1.789772MHz) 클럭으로 비례 분주
    cycle_accumulator_ += static_cast<uint64_t>(cycles) * CLOCK_HZ;
    while (cycle_accumulator_ >= CPU_CLOCK_HZ) {
        cycle_accumulator_ -= CPU_CLOCK_HZ;
        if (ay38910_tick(&pimpl_->ay)) {
            float s = pimpl_->ay.sample;
            // 부드러운 클램핑 (DAC 오버플로우/디지털 클리핑 찢어짐 방지)
            if (s > 0.95f) s = 0.95f;
            else if (s < -0.95f) s = -0.95f;
            sample_buffer_.push_back(s);
        }
    }
}

void PsgAy8910::save_state(std::ostream& os) {
    os.write(reinterpret_cast<const char*>(regs_.data()), regs_.size());
    os.write(reinterpret_cast<const char*>(&selected_reg_), sizeof(selected_reg_));
    os.write(reinterpret_cast<const char*>(&cassette_input_), sizeof(cassette_input_));
}

void PsgAy8910::load_state(std::istream& is) {
    is.read(reinterpret_cast<char*>(regs_.data()), regs_.size());
    is.read(reinterpret_cast<char*>(&selected_reg_), sizeof(selected_reg_));
    is.read(reinterpret_cast<char*>(&cassette_input_), sizeof(cassette_input_));
    if (pimpl_) {
        pimpl_->reset();
        for (uint8_t r = 0; r < 16; ++r) {
            ay38910_set_register(&pimpl_->ay, r, regs_[r]);
        }
        ay38910_set_addr_latch(&pimpl_->ay, selected_reg_);
    }
}

} // namespace spc

