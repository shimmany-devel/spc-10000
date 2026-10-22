#include "cassette.hpp"

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
}

bool Cassette::mount_read_tape(const std::string& filepath) {
    press_stop();
    tape_path_ = filepath;
    read_file_.open(filepath, std::ios::binary);
    if (!read_file_.is_open()) {
        std::cerr << "[Cassette] Failed to open tape file for reading: " << filepath << "\n";
        return false;
    }
    button_ = TapeButton::PLAY;
    std::cout << "[Cassette] Mounted tape for PLAY: " << filepath << "\n";
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
    if (!read_file_.is_open()) {
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
    if (read_file_.is_open()) {
        read_file_.close();
    }
    if (write_file_.is_open()) {
        write_file_.flush();
        write_file_.close();
    }
    std::cout << "[Cassette] Button -> STOP\n";
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

    // 0x80: 테이프 직렬 비트 데이터 입력
    if (button_ == TapeButton::PLAY && motor_on_ && read_ready_) {
        int bit = read_next_bit();
        if (bit == 1) {
            retval |= 0x80;
        } else {
            retval &= ~0x80;
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
    if (!read_file_.is_open() || read_file_.eof()) {
        return 1;
    }
    char c = 0;
    while (read_file_.get(c)) {
        if (c == '0') return 0;
        if (c == '1') return 1;
        // 공백, 줄바꿈 등 기타 문자는 건너뜀
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

    uint32_t len = 0;
    is.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len > 0) {
        tape_path_.resize(len);
        is.read(&tape_path_[0], len);
    } else {
        tape_path_.clear();
    }
}

} // namespace spc
