#include <iostream>
#include <string>
#include "sys_spc1000/spc1000_machine.hpp"

void type_key(spc::Spc1000Machine& machine, SDL_Keycode key, int hold_frames = 5, int release_frames = 5) {
    machine.handle_key_down(key);
    for (int i = 0; i < hold_frames; ++i) machine.run_frame();
    machine.handle_key_up(key);
    for (int i = 0; i < release_frames; ++i) machine.run_frame();
}

void print_screen(spc::Spc1000Machine& machine) {
    const uint8_t gmode = machine.board().vdg().read_gmode();
    const int page = (gmode & 0x30) >> 4;
    const int page_start = page * 0x200;
    std::cout << "--- [Screen Page " << page << "] ---\n";
    for (int r = 0; r < 16; ++r) {
        std::string line;
        for (int c = 0; c < 32; ++c) {
            uint8_t ch = machine.board().vdg().read_vram(page_start + r * 32 + c);
            if (ch >= 32 && ch < 127) line += (char)ch;
            else if (ch == 0) line += ' ';
            else line += '.';
        }
        std::cout << (r < 10 ? "0" : "") << r << ": [" << line << "]\n";
    }
}

int main(int argc, char* argv[]) {
    std::string tap_file = (argc > 1) ? argv[1] : "ADVENTURE.tap";
    std::cout << "[Run Test] Testing LOAD and RUN with: " << tap_file << "\n";

    spc::Spc1000Machine machine;
    if (!machine.init("reference/roms/spc1000.rom")) return 1;
    if (!machine.mount_tape(tap_file)) return 1;

    // Warm-up 60 frames
    for (int i = 0; i < 60; ++i) machine.run_frame();

    std::cout << "1. Typing 'load' + RETURN...\n";
    type_key(machine, SDLK_l);
    type_key(machine, SDLK_o);
    type_key(machine, SDLK_a);
    type_key(machine, SDLK_d);
    type_key(machine, SDLK_RETURN);

    std::cout << "2. Waiting for cassette motor ON...\n";
    int motor_wait = 0;
    while (!machine.is_cassette_motor_on() && motor_wait < 180) {
        machine.run_frame();
        motor_wait++;
    }
    std::cout << "Motor turned ON after " << motor_wait << " frames.\n";

    std::cout << "Loading tape bitstream until motor turns OFF...\n";
    int load_frames = 0;
    while (machine.is_cassette_motor_on() && load_frames < 10000) {
        machine.run_frame();
        load_frames++;
    }
    std::cout << "Loading completed! Total frames: " << load_frames << "\n";

    // 60 프레임 안정화
    for (int i = 0; i < 60; ++i) machine.run_frame();
    print_screen(machine);

    std::cout << "\n3. Typing 'run' + RETURN to start the game...\n";
    type_key(machine, SDLK_r);
    type_key(machine, SDLK_u);
    type_key(machine, SDLK_n);
    type_key(machine, SDLK_RETURN);

    std::cout << "Waiting for secondary block load (if any)...\n";
    int wait_sec = 0;
    while (!machine.is_cassette_motor_on() && wait_sec < 180) {
        machine.run_frame();
        wait_sec++;
    }
    if (machine.is_cassette_motor_on()) {
        std::cout << "Secondary block load detected! Loading until motor turns OFF...\n";
        int load2_frames = 0;
        while (machine.is_cassette_motor_on() && load2_frames < 30000) {
            machine.run_frame();
            load2_frames++;
        }
        std::cout << "Secondary load completed after " << load2_frames << " frames!\n";
    }

    // Run 600 frames to let game display title / prompt
    for (int f = 0; f < 600; ++f) {
        machine.run_frame();
        if ((f + 1) % 150 == 0) {
            std::cout << "\n=== Game Frame " << (f + 1) << " ===\n";
            print_screen(machine);
        }
    }

    return 0;
}
