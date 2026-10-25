#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "sys_spc1000/spc1000_machine.hpp"
#include "sys_spc10000/spc10000_machine.hpp"
#include "ui/emulator_window.hpp"

// Z8000 코어 명령어 및 버스 검증 테스트
bool run_z8000_unit_tests() {
    std::cout << "[Test] Running Z8000 CPU Core Self-Tests...\n";
    spc::Spc10000Board board;
    spc::Z8000Cpu cpu(board, spc::Z8000Cpu::Model::Z8001_Segmented);

    // 1. Z8000 리셋 및 세그먼트 모드 확인
    cpu.reset();
    assert(cpu.is_segmented());
    assert(cpu.regs().fcw & spc::FCW_S_N); // System Mode
    std::cout << "  - Reset & Segmented Mode: PASS\n";

    // 2. 명령어 쓰기 및 실행 테스트:
    //    LD R1, 0x1234   (0x2110 0x1234)
    //    ADD R1, 0x0002  (0x0110 0x0002) -> R1 should be 0x1236
    //    OUTB @0x2000, RH1 (0x3B20 0x2000) -> Out VDG GMODE with 0x12
    uint32_t pc = 0x001000;
    cpu.set_pc_linear(pc);

    board.mem_write_word(pc + 0, 0x2101); // LD R1, 0x1234
    board.mem_write_word(pc + 2, 0x1234);
    board.mem_write_word(pc + 4, 0x0101); // ADD R1, 0x0002
    board.mem_write_word(pc + 6, 0x0002);
    board.mem_write_word(pc + 8, 0x3B02); // OUTB @0x2000, RH1 (RH1 is reg_idx 2)
    board.mem_write_word(pc + 10, 0x2000);

    cpu.step_instruction(); // LD R1, 0x1234
    assert(cpu.regs().r[1] == 0x1234);

    cpu.step_instruction(); // ADD R1, 0x0002
    assert(cpu.regs().r[1] == 0x1236);

    cpu.step_instruction(); // OUTB @0x2000, RH1
    assert(board.vdg().read_gmode() == 0x12);
    std::cout << "  - Basic 16-bit ALU & I/O Bus Bridge: PASS\n";

    // 3. Special I/O (슈퍼 그래픽 모드 토글 테스트)
    //    LDB RL1, 0x01 (Word 1: 0x0C03, Word 2: 0x0001)
    //    SOUTB @0x00, RL1 (Word 1: 0x3903, Word 2: 0x0000) -> Super Graphic ON
    uint32_t pc2 = 0x001020;
    cpu.set_pc_linear(pc2);
    board.mem_write_word(pc2 + 0, 0x0C03); // LDB RL1, imm
    board.mem_write_word(pc2 + 2, 0x0001); // imm = 1
    board.mem_write_word(pc2 + 4, 0x3903); // SOUTB @0x0000, RL1
    board.mem_write_word(pc2 + 6, 0x0000);

    cpu.step_instruction();
    assert(cpu.regs().get_rb(3) == 0x01);
    cpu.step_instruction();
    assert(board.is_super_graphic_mode() == true);
    std::cout << "  - Special I/O & Super Graphic Mode Toggle: PASS\n";

    std::cout << "[Test] All Z8000 CPU unit tests passed successfully!\n";
    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================================\n";
    std::cout << " SPC-1000 Modernization & SPC-10000 Evolution           \n";
    std::cout << " Version: 0.3.0-dev (Dual Z80 & Z8000 Architecture)     \n";
    std::cout << "========================================================\n";

    std::string rom_path = "reference/roms/spc1000.rom";
    std::string tape_path = "";
    std::string rec_tape_path = "";
    std::string load_state_path = "";
    bool headless = false;
    bool run_spc10000 = false;
    int test_frames = 60;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--headless") {
            headless = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                test_frames = std::stoi(argv[++i]);
            }
        } else if (arg == "--spc10000") {
            run_spc10000 = true;
        } else if (arg == "--test-z8000") {
            return run_z8000_unit_tests() ? 0 : 1;
        } else if (arg == "--rom" && i + 1 < argc) {
            rom_path = argv[++i];
        } else if (arg == "--tape" && i + 1 < argc) {
            tape_path = argv[++i];
        } else if (arg == "--record-tape" && i + 1 < argc) {
            rec_tape_path = argv[++i];
        } else if (arg == "--load-state" && i + 1 < argc) {
            load_state_path = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: spc1000 [options]\n"
                      << "  --spc10000              Run in SPC-10000 (16-bit Z8000) mode\n"
                      << "  --rom <path>            Set ROM path (default: reference/roms/spc1000.rom)\n"
                      << "  --tape <path>           Mount virtual cassette tape file (.tap)\n"
                      << "  --record-tape <path>    Mount cassette for recording (.tap)\n"
                      << "  --load-state <path>     Restore saved state snapshot file\n"
                      << "  --headless [frames]     Run without GUI window for N frames\n"
                      << "  --test-z8000            Execute Z8000 CPU self-test suite\n\n"
                      << "Shortcuts in GUI window:\n"
                      << "  F7        : Cassette Tape REWIND\n"
                      << "  F8        : Cassette Tape PLAY\n"
                      << "  F9        : Cassette Tape REC\n"
                      << "  F10       : Cassette Tape STOP\n"
                      << "  F12       : Machine Hardware Reset\n"
                      << "  PageUp    : Save Quick Snapshot (spc1000_snapshot.sav)\n"
                      << "  PageDown  : Load Quick Snapshot (spc1000_snapshot.sav)\n";
            return 0;
        }
    }

    if (run_spc10000) {
        std::cout << "[Mode] Launching SPC-10000 (16-bit Z8000 Evolution)...\n";
        spc::Spc10000Machine machine;
        machine.init("firmware-spc10000/spc10000.rom");
        std::cout << "[System] Z8000 CPU initialized at PC: 0x" 
                  << std::hex << machine.pc() << std::dec << "\n";

        if (headless) {
            std::cout << "[System] Running SPC-10000 in headless mode for " << test_frames << " frames...\n";
            for (int f = 0; f < test_frames; ++f) {
                machine.run_frame();
            }
            std::cout << "[System] Headless execution completed. Final PC: 0x" 
                      << std::hex << machine.pc() << std::dec << "\n";

            std::cout << "\n[SPC-10000 Native Screen Output]:\n";
            for (int r = 0; r < 12; ++r) {
                std::cout << (r < 10 ? " " : "") << r << ": [";
                for (int c = 0; c < 32; ++c) {
                    uint8_t ch = machine.board().vdg().read_vram(r * 32 + c);
                    if (ch >= 32 && ch < 127) {
                        std::cout << (char)ch;
                    } else if (ch == 0) {
                        std::cout << " ";
                    } else {
                        std::cout << ".";
                    }
                }
                std::cout << "]\n";
            }
            return 0;
        }

        spc::EmulatorWindow window;
        if (window.init("SPC-10000 16-bit Computer (Z8001 @ 10MHz)")) {
            std::cout << "[System] Running SPC-10000 GUI display.\n";
            window.run(machine);
        }
        return 0;
    }

    // 기본 SPC-1000 모드
    std::cout << "[Mode] Launching Base SPC-1000 (8-bit Z80A)...\n";
    spc::Spc1000Machine machine;
    if (!machine.init(rom_path)) {
        std::cerr << "[Error] Failed to initialize machine with ROM: " << rom_path << "\n";
        return 1;
    }

    std::cout << "[System] SPC-1000 Hardware initialized. PC: 0x" 
              << std::hex << machine.pc() << std::dec << "\n";

    if (!tape_path.empty()) {
        if (machine.mount_tape(tape_path)) {
            std::cout << "[Cassette] Mounted tape file: " << tape_path << "\n";
        }
    }
    if (!rec_tape_path.empty()) {
        if (machine.record_tape(rec_tape_path)) {
            std::cout << "[Cassette] Mounted recording tape file: " << rec_tape_path << "\n";
        }
    }
    if (!load_state_path.empty()) {
        if (machine.load_snapshot(load_state_path)) {
            std::cout << "[Snapshot] Loaded state snapshot: " << load_state_path << "\n";
        }
    }

    std::cout << "\n[Controls] F7: Rewind | F8: PLAY | F9: REC | F10: STOP | F12: Reset\n"
              << "[Controls] PageUp: Quick Save State | PageDown: Quick Restore State\n\n";

    if (headless) {
        std::cout << "[System] Running in headless test mode for " << test_frames << " frames...\n";
        for (int f = 0; f < test_frames; ++f) {
            machine.run_frame();
            if ((f + 1) % 40 == 0) {
                std::cout << "  Frame " << (f + 1) << "/" << test_frames 
                          << " executed. Current PC: 0x" << std::hex << machine.pc() << std::dec << "\n";
            }
        }
        std::cout << "[System] Headless execution completed successfully.\n";

        const uint8_t gmode = machine.board().vdg().read_gmode();
        const int page = (gmode & 0x30) >> 4;
        std::cout << "[VDG State] GMODE: 0x" << std::hex << (int)gmode << ", Active Text Page: " << page << std::dec << "\n";

        std::cout << "\n[VRAM Non-Zero Summary]:\n";
        int non_zero_count = 0;
        for (size_t a = 0; a < 0x2000; ++a) {
            uint8_t v = machine.board().vdg().read_vram(a);
            if (v != 0) {
                if (non_zero_count < 32) {
                    std::cout << "  Addr 0x" << std::hex << a << ": 0x" << (int)v << std::dec << "\n";
                }
                non_zero_count++;
            }
        }
        std::cout << "  Total non-zero VRAM bytes: " << non_zero_count << "\n";

        std::cout << "\n[SPC-1000 Text Page " << page << " Dump]:\n";
        const int page_start = page * 0x200;
        for (int r = 0; r < 16; ++r) {
            std::cout << (r < 10 ? " " : "") << r << ": [";
            for (int c = 0; c < 32; ++c) {
                uint8_t ch = machine.board().vdg().read_vram(page_start + r * 32 + c);
                if (ch >= 32 && ch < 127) {
                    std::cout << (char)ch;
                } else if (ch == 0) {
                    std::cout << " ";
                } else {
                    std::cout << ".";
                }
            }
            std::cout << "]\n";
        }
        return 0;
    }

    spc::EmulatorWindow window;
    if (!window.init("SPC-1000 Modern Emulator (C++20)")) {
        std::cerr << "[Warning] Display not available. Falling back to headless execution.\n";
        for (int f = 0; f < 60; ++f) machine.run_frame();
        return 0;
    }

    window.run(machine);
    return 0;
}
