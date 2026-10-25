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

        std::cout << "[EmulatorWindow] Entering emulator main loop (60.0 FPS precision pacer)...\n";

        auto next_frame_time = clock::now();

        while (running) {
            next_frame_time += frame_duration;

            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                } else if (event.type == SDL_KEYDOWN) {
                    switch (event.key.keysym.sym) {
                        case SDLK_F7:
                            std::cout << "[EmulatorWindow] Cassette: REWIND\n";
                            machine.cassette_rewind();
                            break;
                        case SDLK_F8:
                            std::cout << "[EmulatorWindow] Cassette: PLAY\n";
                            machine.cassette_play();
                            break;
                        case SDLK_F9:
                            std::cout << "[EmulatorWindow] Cassette: REC\n";
                            machine.cassette_rec();
                            break;
                        case SDLK_F10:
                            std::cout << "[EmulatorWindow] Cassette: STOP\n";
                            machine.cassette_stop();
                            break;
                        case SDLK_F12:
                            std::cout << "[EmulatorWindow] Machine Reset\n";
                            machine.reset();
                            break;
                        case SDLK_PAGEUP:
                            std::cout << "[EmulatorWindow] Saving quick snapshot (spc1000_snapshot.sav)...\n";
                            machine.save_snapshot("spc1000_snapshot.sav");
                            break;
                        case SDLK_PAGEDOWN:
                            std::cout << "[EmulatorWindow] Loading quick snapshot (spc1000_snapshot.sav)...\n";
                            machine.load_snapshot("spc1000_snapshot.sav");
                            break;
                        default:
                            machine.handle_key_down(event.key.keysym.sym);
                            break;
                    }
                } else if (event.type == SDL_KEYUP) {
                    machine.handle_key_up(event.key.keysym.sym);
                }
            }

            // 카세트 모터 작동 중(LOAD/SAVE)에는 초고속 터보 모드 (16배속 및 딜레이 스킵)
            int extra_frames = machine.is_cassette_motor_on() ? 15 : 0;
            machine.run_frame();
            for (int i = 0; i < extra_frames && machine.is_cassette_motor_on(); ++i) {
                machine.run_frame();
            }

            // 화면 렌더링
            machine.render(framebuffer_.data(), NATIVE_WIDTH);

            SDL_UpdateTexture(texture_, nullptr, framebuffer_.data(), NATIVE_WIDTH * sizeof(uint32_t));
            SDL_RenderClear(renderer_);
            SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
            SDL_RenderPresent(renderer_);

            // 실시간 오디오 샘플 큐잉
            if (audio_device_ > 0) {
                if (machine.is_cassette_motor_on()) {
                    SDL_ClearQueuedAudio(audio_device_);
                    machine.board().psg().clear_samples();
                } else {
                    const auto& samples = machine.board().psg().sample_buffer();
                    if (!samples.empty()) {
                        uint32_t queued = SDL_GetQueuedAudioSize(audio_device_);
                        if (queued < (44100 * sizeof(float)) / 10) {
                            SDL_QueueAudio(audio_device_, samples.data(), samples.size() * sizeof(float));
                        }
                        machine.board().psg().clear_samples();
                    }
                }
            }

            // 정밀 하이브리드 프레임 페이싱 (카세트 터보 중이 아닐 때)
            if (machine.is_cassette_motor_on()) {
                next_frame_time = clock::now();
            } else {
                auto now = clock::now();
                if (now < next_frame_time) {
                    auto remaining = next_frame_time - now;
                    if (remaining > std::chrono::milliseconds(2)) {
                        std::this_thread::sleep_for(remaining - std::chrono::milliseconds(1));
                    }
                    while (clock::now() < next_frame_time) {
                        std::this_thread::yield();
                    }
                } else if (now > next_frame_time + frame_duration * 3) {
                    next_frame_time = now;
                }
            }
        }
    }

private:
    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};
    SDL_Texture* texture_{nullptr};
    SDL_AudioDeviceID audio_device_{0};
    std::vector<uint32_t> framebuffer_;
};

} // namespace spc
