#include "emulator_window.hpp"
#include <iostream>

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

} // namespace spc
