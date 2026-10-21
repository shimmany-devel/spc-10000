#pragma once
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include "../sys_spc1000/spc1000_machine.hpp"

namespace spc {

class EmulatorWindow {
public:
    static constexpr int NATIVE_WIDTH = 256;
    static constexpr int NATIVE_HEIGHT = 192;
    static constexpr int SCALE = 3;
    static constexpr int WINDOW_WIDTH = NATIVE_WIDTH * SCALE;   // 768
    static constexpr int WINDOW_HEIGHT = NATIVE_HEIGHT * SCALE; // 576

    EmulatorWindow();
    ~EmulatorWindow();

    bool init(const std::string& title);
    void run(Spc1000Machine& machine);

private:
    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};
    SDL_Texture* texture_{nullptr};
    std::vector<uint32_t> framebuffer_;
};

} // namespace spc
