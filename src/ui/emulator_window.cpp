#include "emulator_window.hpp"
#include <iostream>
#include <chrono>
#include <thread>

namespace spc {

EmulatorWindow::EmulatorWindow() {
    framebuffer_.resize(NATIVE_WIDTH * NATIVE_HEIGHT, 0xFF000000);
}

EmulatorWindow::~EmulatorWindow() {
    if (texture_) SDL_DestroyTexture(texture_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
}

bool EmulatorWindow::init(const std::string& title) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        std::cerr << "[EmulatorWindow] SDL_Init Error: " << SDL_GetError() << "\n";
        return false;
    }

    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window_) {
        std::cerr << "[EmulatorWindow] SDL_CreateWindow Error: " << SDL_GetError() << "\n";
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }

    if (!renderer_) {
        std::cerr << "[EmulatorWindow] SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
        return false;
    }

    texture_ = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        NATIVE_WIDTH,
        NATIVE_HEIGHT
    );

    if (!texture_) {
        std::cerr << "[EmulatorWindow] SDL_CreateTexture Error: " << SDL_GetError() << "\n";
        return false;
    }

    return true;
}

void EmulatorWindow::run(Spc1000Machine& machine) {
    bool running = true;
    SDL_Event event;

    using clock = std::chrono::steady_clock;
    const std::chrono::nanoseconds frame_duration(1000000000 / Spc1000Machine::FPS);

    std::cout << "[EmulatorWindow] Entering emulator main loop (60 FPS)...\n";

    while (running) {
        auto frame_start = clock::now();

        // 1. 이벤트 폴링
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

        // 2. 60Hz 1프레임 에뮬레이션 구동
        machine.run_frame();

        // 3. 화면 렌더링
        machine.render(framebuffer_.data(), NATIVE_WIDTH);

        SDL_UpdateTexture(texture_, nullptr, framebuffer_.data(), NATIVE_WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer_);
        SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
        SDL_RenderPresent(renderer_);

        // 4. 60FPS 프레임 레이트 조절
        auto frame_end = clock::now();
        auto elapsed = frame_end - frame_start;
        if (elapsed < frame_duration) {
            std::this_thread::sleep_for(frame_duration - elapsed);
        }
    }
}

} // namespace spc
