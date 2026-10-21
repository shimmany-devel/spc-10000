/*
 * debug_shell - Interactive debugger & monitor for SPC-10000 on ZCU102
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define REG_BASE_ADDR   0xA0010000
#define BRAM_BASE_ADDR  0xA0000000
#define MAP_SIZE        0x10000

static volatile uint32_t *regs = NULL;
static volatile uint8_t *bram = NULL;

void print_help(void) {
    printf("Commands:\n");
    printf("  r             : Read system status & registers\n");
    printf("  d <addr> <len>: Dump memory from BRAM\n");
    printf("  w <addr> <val>: Write byte to BRAM\n");
    printf("  reset [0|1]   : Hold (1) or Release (0) CPU Reset\n");
    printf("  mode [0|1]    : Select 8-bit Z80 (0) or 16-bit Z8000 (1)\n");
    printf("  speed <0-3>   : Set clock speed (0:10M, 1:20M, 2:50M, 3:100M)\n");
    printf("  q             : Quit\n");
}

int main(void) {
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

    printf("SPC-10000 Hardware Debug Shell. Type 'help' for commands.\n");
    char cmd[128];

    while (1) {
        printf("spc10000> ");
        if (!fgets(cmd, sizeof(cmd), stdin)) break;

        if (strncmp(cmd, "help", 4) == 0) {
            print_help();
        } else if (strncmp(cmd, "r", 1) == 0 && (cmd[1] == '\n' || cmd[1] == ' ')) {
            uint32_t ctrl = regs[0];
            uint32_t stat = regs[1];
            printf("SYS_CTRL : 0x%08X (Reset=%d, Mode=%s, Speed=%d)\n",
                   ctrl, ctrl & 1, (ctrl & 2) ? "Z8000" : "Z80", (ctrl >> 2) & 3);
            printf("STATUS   : 0x%08X\n", stat);
        } else if (strncmp(cmd, "d ", 2) == 0) {
            unsigned int addr, len;
            if (sscanf(cmd + 2, "%x %x", &addr, &len) == 2) {
                for (unsigned int i = 0; i < len; i += 16) {
                    printf("%06X: ", addr + i);
                    for (unsigned int j = 0; j < 16 && (i + j) < len; j++) {
                        printf("%02X ", bram[addr + i + j]);
                    }
                    printf("\n");
                }
            } else {
                printf("Usage: d <hex_addr> <hex_len>\n");
            }
        } else if (strncmp(cmd, "q", 1) == 0) {
            break;
        }
    }

    close(mem_fd);
    return 0;
}
