#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <string>

#define CHIPS_UTIL_IMPL
#include "../reference/chips-repo/util/z80dasm.h"

struct DasmContext {
    const std::vector<uint8_t>& rom;
    uint16_t cur_pc;
    std::string line;
};

static uint8_t in_cb(void* user_data) {
    auto* ctx = static_cast<DasmContext*>(user_data);
    if (ctx->cur_pc < ctx->rom.size()) {
        return ctx->rom[ctx->cur_pc++];
    }
    return 0;
}

static void out_cb(char c, void* user_data) {
    auto* ctx = static_cast<DasmContext*>(user_data);
    ctx->line += c;
}

void disassemble_range(const std::vector<uint8_t>& rom, uint16_t start, uint16_t end) {
    DasmContext ctx{rom, start, ""};
    uint16_t pc = start;
    while (pc < end && pc < rom.size()) {
        ctx.cur_pc = pc;
        ctx.line.clear();
        uint16_t old_pc = pc;
        pc = z80dasm_op(pc, in_cb, out_cb, &ctx);
        std::cout << "$" << std::hex << std::setw(4) << std::setfill('0') << old_pc << ": ";
        // Print raw bytes
        for (uint16_t i = old_pc; i < pc; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)rom[i] << " ";
        }
        for (int i = 0; i < (4 - (pc - old_pc)); ++i) std::cout << "   ";
        std::cout << " " << ctx.line << "\n";
    }
}

int main(int argc, char* argv[]) {
    std::ifstream f("reference/roms/spc1000.rom", std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "Failed to open ROM\n";
        return 1;
    }
    std::vector<uint8_t> rom(32768);
    f.read(reinterpret_cast<char*>(rom.data()), 32768);
    std::cout << "=== Vectors (0x0000 - 0x0080) ===\n";
    disassemble_range(rom, 0x0000, 0x0080);

    std::cout << "\n=== Startup Routine (0x06A4) ===\n";
    disassemble_range(rom, 0x0D90, 0x0DC0);

    return 0;
}
