#pragma once
#include <cstdint>
#include <array>
#include <iosfwd>
#include <vector>

namespace spc {

class VdgMc6847 {
public:
    static constexpr int SCREEN_WIDTH = 256;
    static constexpr int SCREEN_HEIGHT = 192;
    static constexpr size_t VRAM_SIZE = 0x2000; // 8KB

    VdgMc6847();

    void reset();

    // VRAM 읽기/쓰기 (포트 0x0000 ~ 0x1FFF)
    uint8_t read_vram(uint16_t addr) const;
    void write_vram(uint16_t addr, uint8_t val);

    // GMODE 제어 (포트 0x2000 ~ 0x3FFF)
    uint8_t read_gmode() const;
    void write_gmode(uint8_t val);

    // 256x192 RGBA 픽셀 버퍼로 현재 화면 렌더링
    void render(uint32_t* framebuffer, int pitch_pixels);

    // VRAM 직접 접근 포인터
    uint8_t* vram_data() { return vram_.data(); }

    void save_state(std::ostream& os);
    void load_state(std::istream& is);

private:
    void render_text_mode(uint32_t* fb, int pitch);
    void render_graphic_mode(uint32_t* fb, int pitch);

    std::array<uint8_t, VRAM_SIZE> vram_{};
    uint8_t gmode_{0};
};

} // namespace spc
