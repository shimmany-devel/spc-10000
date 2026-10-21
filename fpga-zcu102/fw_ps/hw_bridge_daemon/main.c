/*
 * hw_bridge_daemon - ZCU102 PS-side Hardware Bridge Daemon
 * Handles USB Keyboard event injection and ROM/RAM loading via /dev/mem
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/input.h>

#define REG_BASE_ADDR   0xA0010000
#define BRAM_BASE_ADDR  0xA0000000
#define MAP_SIZE        0x10000

#define OFFSET_SYS_CTRL 0x00
#define OFFSET_STATUS   0x04
#define OFFSET_KBD_0_3  0x10
#define OFFSET_KBD_4_7  0x14
#define OFFSET_KBD_8_9  0x18
#define OFFSET_MBOX     0x20

static volatile uint32_t *regs = NULL;
static volatile uint8_t *bram = NULL;

int load_rom(const char *filename, uint32_t dest_offset) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Failed to open ROM file");
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    printf("[Bridge] Loading %ld bytes from %s to BRAM offset 0x%06X...\n", sz, filename, dest_offset);
    fread((void*)(bram + dest_offset), 1, sz, fp);
    fclose(fp);
    return 0;
}

int main(int argc, char *argv[]) {
    printf("=========================================\n");
    printf("  SPC-10000 ZCU102 Hardware Bridge Daemon\n");
    printf("=========================================\n");

    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("Error opening /dev/mem (requires root)");
        return 1;
    }

    regs = (volatile uint32_t *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, REG_BASE_ADDR);
    bram = (volatile uint8_t *)mmap(NULL, MAP_SIZE * 4, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, BRAM_BASE_ADDR);

    if (regs == MAP_FAILED || bram == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    // Hold CPU in reset
    regs[OFFSET_SYS_CTRL / 4] = 0x00000001;
    printf("[Bridge] CPU placed in RESET mode.\n");

    if (argc >= 2) {
        load_rom(argv[1], 0x000000);
    }

    // Set Mode: Bit 1 (0: Z80, 1: Z8000), Speed: Bit 3:2 (0: 10MHz, 1: 20MHz, 2: 50MHz, 3: 100MHz)
    int is_z8000 = (argc >= 3 && strcmp(argv[2], "z8000") == 0);
    uint32_t ctrl_val = (is_z8000 ? 0x02 : 0x00) | (0 << 2); // 10MHz, not in reset (bit 0 = 0)
    
    // Release Reset
    regs[OFFSET_SYS_CTRL / 4] = ctrl_val;
    printf("[Bridge] Released RESET! CPU running in %s mode.\n", is_z8000 ? "Z8000 (SPC-10000)" : "Z80A (SPC-1000)");

    // Initialize Keyboard Matrix (all keys unpressed = 0xFF)
    regs[OFFSET_KBD_0_3 / 4] = 0xFFFFFFFF;
    regs[OFFSET_KBD_4_7 / 4] = 0xFFFFFFFF;
    regs[OFFSET_KBD_8_9 / 4] = 0x0000FFFF;

    printf("[Bridge] Ready. Monitoring input...\n");

    close(mem_fd);
    return 0;
}
