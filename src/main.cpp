#include <iostream>
#include <string>
#include <vector>
#include "sys_spc1000/spc1000_machine.hpp"
#include "ui/emulator_window.hpp"

int main(int argc, char* argv[]) {
    std::cout << "========================================================\n";
    std::cout << " SPC-1000 Modernization & SPC-10000 Evolution           \n";
    std::cout << " Phase 2: Modular C++20 SPC-1000 Base Emulator          \n";
    std::cout << "========================================================\n";

    std::string rom_path = "reference/roms/spc1000.rom";
    bool headless = false;
    int test_frames = 60;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--headless") {
            headless = true;
            if (i + 1 < argc) {
                test_frames = std::stoi(argv[++i]);
            }
        } else if (arg == "--rom" && i + 1 < argc) {
            rom_path = argv[++i];
        }
    }

    spc::Spc1000Machine machine;
    if (!machine.init(rom_path)) {
        std::cerr << "[Error] Failed to initialize machine with ROM: " << rom_path << "\n";
        return 1;
    }

    std::cout << "[System] SPC-1000 Hardware initialized. PC: 0x" 
              << std::hex << machine.pc() << std::dec << "\n";

    if (headless) {
        std::cout << "[System] Running in headless test mode for " << test_frames << " frames...\n";
        for (int f = 0; f < test_frames; ++f) {
            if (f == 40) {
                std::cout << "  [Input Simulation] Sending ENTER (SDLK_RETURN) key press...\n";
                machine.handle_key_down(SDLK_RETURN);
            }
            if (f == 50) {
                machine.handle_key_up(SDLK_RETURN);
            }
            machine.run_frame();
            if ((f + 1) % 20 == 0) {
                std::cout << "  Frame " << (f + 1) << "/" << test_frames 
                          << " executed. Current PC: 0x" << std::hex << machine.pc() << std::dec << "\n";
            }
        }
        std::cout << "[System] Headless execution completed successfully.\n";

        // VRAM 텍스트 화면 상태 출력 검증 (16줄 전체)
        const uint8_t gmode = machine.board().vdg().read_gmode();
        const int page = (gmode & 0x30) >> 4;
        std::cout << "[VDG State] GMODE: 0x" << std::hex << (int)gmode << ", Active Text Page: " << page << std::dec << "\n";

        int non_zero_count = 0;
        for (size_t i = 0; i < spc::VdgMc6847::VRAM_SIZE; ++i) {
            if (machine.board().vdg().read_vram(i) != 0) {
                non_zero_count++;
            }
        }
        std::cout << "[VRAM Status] Total non-zero bytes across 8KB VRAM: " << non_zero_count << "\n";
        std::cout << "[VRAM Non-Zero Regions]:\n";
        for (size_t block = 0; block < spc::VdgMc6847::VRAM_SIZE; block += 512) {
            int cnt = 0;
            for (size_t i = 0; i < 512; ++i) {
                if (machine.board().vdg().read_vram(block + i) != 0) cnt++;
            }
            if (cnt > 0) {
                std::cout << "  Block 0x" << std::hex << block << " - 0x" << (block + 511)
                          << ": " << std::dec << cnt << " bytes\n";
            }
        }
        std::cout << "[VRAM Text Screen (16 lines)]:\n";
        for (int r = 0; r < 16; ++r) {
            std::cout << (r < 10 ? " " : "") << r << ": [";
            for (int c = 0; c < 32; ++c) {
                uint8_t ch = machine.board().vdg().read_vram(r * 32 + c);
                if (ch >= 32 && ch < 127) {
                    std::cout << (char)ch;
                } else if (ch == 0) {
                    std::cout << " ";
                } else {
                    std::cout << "~";
                }
            }
            std::cout << "]\n";
        }
        return 0;
    }

    // GUI 윈도우 실행 모드
    spc::EmulatorWindow window;
    if (!window.init("SPC-1000 Modern Emulator (C++20)")) {
        std::cerr << "[Warning] Display not available or SDL init failed. Falling back to headless test...\n";
        for (int f = 0; f < 60; ++f) {
            machine.run_frame();
        }
        std::cout << "[System] Headless fallback frame execution finished. Final PC: 0x" 
                  << std::hex << machine.pc() << std::dec << "\n";
        return 0;
    }

    window.run(machine);
    return 0;
}
