#include "cassette.hpp"
#include <vector>
#include <algorithm>

namespace spc {

Cassette::Cassette() {
    reset();
}

Cassette::~Cassette() {
    press_stop();
}

void Cassette::reset() {
    press_stop();
    motor_on_ = false;
    pulse_ = false;
    read_ready_ = false;
    control_val_ = 0;
    read_phase_ = ReadPhase::EDGE_LOW;
    current_bit_ = 1;
    bit_index_ = 0;
}

bool Cassette::mount_read_tape(const std::string& filepath) {
    press_stop();
    tape_path_ = filepath;
    tape_bits_.clear();
    bit_index_ = 0;
    read_phase_ = ReadPhase::EDGE_LOW;

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[Cassette] Failed to open tape file for reading: " << filepath << "\n";
        return false;
    }

    std::vector<uint8_t> file_bytes((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());
    file.close();

    if (file_bytes.empty()) {
        std::cerr << "[Cassette] Tape file is empty: " << filepath << "\n";
        return false;
    }

    // CAS 포맷 확인 (16바이트 매직 헤더: "SPC-1000.CASfmt ")
    bool is_cas = false;
    if (file_bytes.size() >= 16) {
        std::string magic(reinterpret_cast<const char*>(file_bytes.data()), 16);
        if (magic == "SPC-1000.CASfmt ") {
            is_cas = true;
        }
    }

    if (is_cas) {
        // CAS 포맷: 헤더 16바이트 제외, 각 바이트를 상위 비트부터 (Bit 7 -> Bit 0) 풀어서 비트 배열로 저장
        tape_bits_.reserve((file_bytes.size() - 16) * 8);
        for (size_t i = 16; i < file_bytes.size(); ++i) {
            uint8_t b = file_bytes[i];
            for (int j = 7; j >= 0; --j) {
                tape_bits_.push_back((b >> j) & 1);
            }
        }
        std::cout << "[Cassette] Mounted .CAS tape: " << filepath 
                  << " (" << tape_bits_.size() << " bits, " << (file_bytes.size() - 16) << " bytes payload)\n";
    } else {
        // TAP 포맷: '0'(0x30)과 '1'(0x31) 텍스트 문자로 구성
        tape_bits_.reserve(file_bytes.size());
        for (uint8_t c : file_bytes) {
            if (c == '0') {
                tape_bits_.push_back(0);
            } else if (c == '1') {
                tape_bits_.push_back(1);
            }
        }
        std::cout << "[Cassette] Mounted .TAP tape: " << filepath 
                  << " (" << tape_bits_.size() << " bits)\n";
    }

    button_ = TapeButton::PLAY;
    return true;
}

bool Cassette::mount_write_tape(const std::string& filepath) {
    press_stop();
    tape_path_ = filepath;
    write_file_.open(filepath, std::ios::binary);
    if (!write_file_.is_open()) {
        std::cerr << "[Cassette] Failed to open tape file for writing: " << filepath << "\n";
        return false;
    }
    button_ = TapeButton::REC;
    std::cout << "[Cassette] Mounted tape for REC: " << filepath << "\n";
    return true;
}

void Cassette::press_play() {
    if (button_ == TapeButton::REC) {
        press_stop();
    }
    if (tape_bits_.empty() && !tape_path_.empty()) {
        mount_read_tape(tape_path_);
    } else {
        button_ = TapeButton::PLAY;
        std::cout << "[Cassette] Button -> PLAY\n";
    }
}

void Cassette::press_rec() {
    if (button_ == TapeButton::PLAY) {
        press_stop();
    }
    if (!write_file_.is_open()) {
        mount_write_tape(tape_path_.empty() ? "saved_program.tap" : tape_path_);
    } else {
        button_ = TapeButton::REC;
        std::cout << "[Cassette] Button -> REC\n";
    }
}

void Cassette::press_stop() {
    button_ = TapeButton::STOP;
    motor_on_ = false;
    if (write_file_.is_open()) {
        write_file_.flush();
        write_file_.close();
    }
    std::cout << "[Cassette] Button -> STOP\n";
}

void Cassette::rewind() {
    bit_index_ = 0;
    read_phase_ = ReadPhase::EDGE_LOW;
    std::cout << "[Cassette] Tape rewound to beginning.\n";
}

void Cassette::write_control(uint8_t val) {
    control_val_ = val;
    read_ready_ = (val & 0x20) != 0;

    // 모터 펄스 토글 (bit 1: 0 -> 1 -> 0 변화 시 모터 반전)
    if (button_ != TapeButton::STOP) {
        if (val & 0x02) {
            pulse_ = true;
        } else {
            if (pulse_) {
                pulse_ = false;
                motor_on_ = !motor_on_;
                std::cout << "[Cassette] Motor " << (motor_on_ ? "ON" : "OFF") 
                          << " (Mode: " << (button_ == TapeButton::PLAY ? "PLAY" : "REC") << ")\n";
                if (!motor_on_ && button_ == TapeButton::REC) {
                    if (write_file_.is_open()) {
                        write_file_.flush();
                    }
                }
            }
        }
    }

    // 테이프에 직렬 비트 쓰기 (bit 7이 스트로브, bit 0이 데이터)
    if (val & 0x80) {
        if (button_ == TapeButton::REC && motor_on_ && write_file_.is_open()) {
            char bit_char = (val & 0x01) ? '1' : '0';
            write_file_.put(bit_char);
        }
    }
}

uint8_t Cassette::read_io() {
    uint8_t retval = 0x1F;

    // 0x80: 테이프 직렬 비트 데이터 입력 (상승 엣지 FSM)
    if (button_ == TapeButton::PLAY && motor_on_) {
        switch (read_phase_) {
            case ReadPhase::EDGE_LOW:
                // 1단계: 0x023B 하강 레벨 감지 (Bit 7 = 0)
                retval &= ~0x80;
                read_phase_ = ReadPhase::EDGE_HIGH;
                break;
            case ReadPhase::EDGE_HIGH:
                // 2단계: 0x024F 상승 엣지 감지 (Bit 7 = 1) -> 0x0230 루틴 통과!
                retval |= 0x80;
                read_phase_ = ReadPhase::DATA_BIT;
                break;
            case ReadPhase::DATA_BIT:
                // 3단계: 0x026E 실제 데이터 비트 샘플링
                current_bit_ = read_next_bit();
                if (current_bit_ == 1) {
                    retval |= 0x80;
                } else {
                    retval &= ~0x80;
                }
                read_phase_ = ReadPhase::EDGE_LOW;
                break;
        }
    }

    // 0x40: 모터 상태 (0: Motor ON, 1: Motor OFF)
    if (motor_on_) {
        retval &= ~0x40;
    } else {
        retval |= 0x40;
    }

    return retval;
}

int Cassette::read_next_bit() {
    if (bit_index_ < tape_bits_.size()) {
        return tape_bits_[bit_index_++];
    }
    return 1;
}

void Cassette::save_state(std::ostream& os) {
    uint32_t btn = static_cast<uint32_t>(button_);
    os.write(reinterpret_cast<const char*>(&btn), sizeof(btn));
    os.write(reinterpret_cast<const char*>(&motor_on_), sizeof(motor_on_));
    os.write(reinterpret_cast<const char*>(&pulse_), sizeof(pulse_));
    os.write(reinterpret_cast<const char*>(&read_ready_), sizeof(read_ready_));
    os.write(reinterpret_cast<const char*>(&control_val_), sizeof(control_val_));

    uint64_t bidx = static_cast<uint64_t>(bit_index_);
    os.write(reinterpret_cast<const char*>(&bidx), sizeof(bidx));

    uint32_t len = static_cast<uint32_t>(tape_path_.size());
    os.write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0) {
        os.write(tape_path_.data(), len);
    }
}

void Cassette::load_state(std::istream& is) {
    uint32_t btn = 0;
    is.read(reinterpret_cast<char*>(&btn), sizeof(btn));
    button_ = static_cast<TapeButton>(btn);
    is.read(reinterpret_cast<char*>(&motor_on_), sizeof(motor_on_));
    is.read(reinterpret_cast<char*>(&pulse_), sizeof(pulse_));
    is.read(reinterpret_cast<char*>(&read_ready_), sizeof(read_ready_));
    is.read(reinterpret_cast<char*>(&control_val_), sizeof(control_val_));

    uint64_t bidx = 0;
    is.read(reinterpret_cast<char*>(&bidx), sizeof(bidx));

    uint32_t len = 0;
    is.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len > 0) {
        tape_path_.resize(len);
        is.read(&tape_path_[0], len);
        mount_read_tape(tape_path_);
        bit_index_ = static_cast<size_t>(bidx);
    } else {
        tape_path_.clear();
        tape_bits_.clear();
        bit_index_ = 0;
    }
}

} // namespace spc
