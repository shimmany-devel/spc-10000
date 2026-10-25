#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

namespace spc {

enum class TapeButton {
    STOP,
    PLAY,
    REC
};

class Cassette {
public:
    Cassette();
    ~Cassette();

    void reset();

    // 카세트 테이프 마운트 및 버튼 제어 (.tap 및 .cas 포맷 지원)
    bool mount_read_tape(const std::string& filepath);
    bool mount_write_tape(const std::string& filepath);
    void press_play();
    void press_rec();
    void press_stop();
    void rewind();

    // 포트 0x6000 쓰기 (모터 및 직렬 비트 출력)
    void write_control(uint8_t val);

    // 포트 0x4001 읽기 (레지스터 14 - 모터 상태 및 직렬 비트 입력)
    uint8_t read_io();

    bool is_motor_on() const { return motor_on_; }
    TapeButton current_button() const { return button_; }
    const std::string& current_tape_path() const { return tape_path_; }
    size_t tape_bits_count() const { return tape_bits_.size(); }
    size_t current_bit_index() const { return bit_index_; }

    // 스냅샷 직렬화
    void save_state(std::ostream& os);
    void load_state(std::istream& is);

private:
    int read_next_bit();

    enum class ReadPhase {
        EDGE_LOW,
        EDGE_HIGH,
        DATA_BIT
    };
    ReadPhase read_phase_{ReadPhase::EDGE_LOW};
    int current_bit_{1};

    TapeButton button_{TapeButton::STOP};
    bool motor_on_{false};
    bool pulse_{false};
    bool read_ready_{false};
    uint8_t control_val_{0};

    std::string tape_path_{"tape.tap"};
    std::vector<uint8_t> tape_bits_;
    size_t bit_index_{0};
    std::ofstream write_file_;
};

} // namespace spc
