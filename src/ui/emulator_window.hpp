#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>
#include <SDL2/SDL.h>

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

    template<typename MachineType>
    void run(MachineType& machine) {
        bool running = true;
        SDL_Event event;

        using clock = std::chrono::steady_clock;
        const std::chrono::nanoseconds frame_duration(1000000000 / 60);

        std::cout << "[EmulatorWindow] Entering emulator main loop (60 FPS)...\n";

        while (running) {
            auto frame_start = clock::now();

            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                } else if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_F10) {
                        machine.reset();
                        std::cout << "[EmulatorWindow] Machine Reset\n";
                    } else {
                        machine.handle_key_down(event.key.keysym.sym);
                    }
                } else if (event.type == SDL_KEYUP) {
                    machine.handle_key_up(event.key.keysym.sym);
                }
            }

            // 60Hz 1프레임 에뮬레이션
            machine.run_frame();

            // 화면 렌더링
            machine.render(framebuffer_.data(), NATIVE_WIDTH);

            SDL_UpdateTexture(texture_, nullptr, framebuffer_.data(), NATIVE_WIDTH * sizeof(uint32_t));
            SDL_RenderClear(renderer_);
            SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
            SDL_RenderPresent(renderer_);

            auto frame_end = clock::now();
            auto elapsed = frame_end - frame_start;
            if (elapsed < frame_duration) {
                std::this_thread::sleep_for(frame_duration - elapsed);
            }
        }
    }

private:
    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};
    SDL_Texture* texture_{nullptr};
    std::vector<uint32_t> framebuffer_;
};

} // namespace spc
