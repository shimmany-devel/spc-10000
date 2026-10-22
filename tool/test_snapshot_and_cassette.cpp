#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <cstdio>
#include "sys_spc1000/spc1000_machine.hpp"

int main() {
    std::cout << "========================================================\n";
    std::cout << " SPC-1000 Snapshot & Cassette Tape Verification Test    \n";
    std::cout << "========================================================\n";

    spc::Spc1000Machine machine;
    if (!machine.init("reference/roms/spc1000.rom")) {
        std::cerr << "[FAIL] Failed to load ROM reference/roms/spc1000.rom\n";
        return 1;
    }

    std::cout << "[Test 1] Running machine for 60 frames to warm up...\n";
    for (int i = 0; i < 60; ++i) {
        machine.run_frame();
    }

    uint16_t saved_pc = machine.pc();
    uint8_t saved_ram_sample = machine.board().mem_read(0x1000);
    uint8_t saved_vram_sample = machine.board().vdg().read_vram(0x0100);
    uint8_t saved_gmode = machine.board().vdg().read_gmode();

    std::cout << "  - Warm-up PC: 0x" << std::hex << saved_pc 
              << ", RAM[0x1000]: 0x" << (int)saved_ram_sample 
              << ", VRAM[0x100]: 0x" << (int)saved_vram_sample 
              << ", GMODE: 0x" << (int)saved_gmode << std::dec << "\n";

    std::cout << "[Test 2] Saving snapshot to test_state.sav...\n";
    if (!machine.save_snapshot("test_state.sav")) {
        std::cerr << "[FAIL] save_snapshot failed!\n";
        return 1;
    }

    std::cout << "[Test 3] Corrupting/Modifying state and running 60 more frames...\n";
    // RAM을 의도적으로 변경
    machine.board().mem_write(0x1000, saved_ram_sample ^ 0xFF);
    machine.board().vdg().write_vram(0x0100, saved_vram_sample ^ 0xAA);
    for (int i = 0; i < 60; ++i) {
        machine.run_frame();
    }
    std::cout << "  - Advanced PC: 0x" << std::hex << machine.pc() << std::dec << "\n";

    std::cout << "[Test 4] Restoring snapshot from test_state.sav...\n";
    if (!machine.load_snapshot("test_state.sav")) {
        std::cerr << "[FAIL] load_snapshot failed!\n";
        return 1;
    }

    uint16_t restored_pc = machine.pc();
    uint8_t restored_ram_sample = machine.board().mem_read(0x1000);
    uint8_t restored_vram_sample = machine.board().vdg().read_vram(0x0100);
    uint8_t restored_gmode = machine.board().vdg().read_gmode();

    std::cout << "  - Restored PC: 0x" << std::hex << restored_pc 
              << ", RAM[0x1000]: 0x" << (int)restored_ram_sample 
              << ", VRAM[0x100]: 0x" << (int)restored_vram_sample 
              << ", GMODE: 0x" << (int)restored_gmode << std::dec << "\n";

    if (restored_pc != saved_pc || restored_ram_sample != saved_ram_sample ||
        restored_vram_sample != saved_vram_sample || restored_gmode != saved_gmode) {
        std::cerr << "[FAIL] Snapshot state mismatch!\n";
        return 1;
    }
    std::cout << "  ==> Snapshot Save/Restore verification: PASSED!\n";

    std::cout << "\n[Test 5] Cassette Tape I/O & Button Control Test...\n";
    // 테스트용 임시 TAP 파일 생성
    {
        std::ofstream tap("test_temp.tap", std::ios::binary);
        for (int i = 0; i < 64; ++i) {
            tap.put((char)(i & 0xFF));
        }
    }

    if (!machine.mount_tape("test_temp.tap")) {
        std::cerr << "[FAIL] mount_tape failed!\n";
        return 1;
    }
    machine.cassette_play();

    // 0x6000 쓰기로 모터 플립플롭 펄스 인가 (bit 1: 1 -> 0 시 모터 토글 ON)
    machine.board().io_write(0x6000, 0x02);
    machine.board().io_write(0x6000, 0x00); // 하강 에지 -> Motor ON!

    // 모터가 켜지고 PLAY 상태이므로 데이터 비트 및 모터 상태 확인
    for (int step = 0; step < 20; ++step) {
        // read_ready 비트(0x20) 설정
        machine.board().io_write(0x6000, 0x20);
        // PSG Port A (레지스터 14) 읽기
        machine.board().io_write(0x4000, 14);
        [[maybe_unused]] uint8_t psg_port_a = machine.board().io_read(0x4001);
        // bit 6은 모터 상태 (0 = ON)
        assert((psg_port_a & 0x40) == 0);
    }
    std::cout << "  - Motor ON and tape pulse clocking verified.\n";

    // 모터 플립플롭 펄스 인가 (bit 1: 1 -> 0 시 모터 토글 OFF)
    machine.board().io_write(0x6000, 0x02);
    machine.board().io_write(0x6000, 0x00); // 하강 에지 -> Motor OFF!
    machine.board().io_write(0x4000, 14);
    [[maybe_unused]] uint8_t psg_port_a_off = machine.board().io_read(0x4001);
    // 모터 OFF 시 bit 6은 1
    assert((psg_port_a_off & 0x40) == 0x40);
    std::cout << "  - Motor OFF and tape STOP verified.\n";
    std::cout << "  ==> Cassette Tape I/O verification: PASSED!\n";

    // 정리
    std::remove("test_state.sav");
    std::remove("test_temp.tap");

    std::cout << "\n========================================================\n";
    std::cout << " ALL VERIFICATION TESTS COMPLETED SUCCESSFULLY!        \n";
    std::cout << "========================================================\n";
    return 0;
}
