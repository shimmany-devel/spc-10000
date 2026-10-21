/**
 * SPC-1000 ROM Decompiled & Reconstructed C Source
 * Module: bootstrap.c (System Reset & Initialization Sequence)
 * Target ROM: spc1000.rom (Offset 0x004A - 0x007F)
 */

#include <stdint.h>
#include <stdbool.h>

/* I/O Port Definitions */
#define PORT_VDG_GMODE   0x2000
#define PORT_AY8910_REG  0x4000
#define PORT_AY8910_DATA 0x4001
#define PORT_CASSETTE    0x6000
#define PORT_IPLK_TOGGLE 0xA000

/* Hardware System Variables in RAM */
#define VAR_CAS_MOTOR    (*(volatile uint8_t*)0x11E9)
#define VAR_VDG_GMODE    (*(volatile uint8_t*)0x11EA)
#define VAR_TIMER_TICK   (*(volatile uint16_t*)0x0036)

/* External Subroutines in ROM */
extern void vram_clear_screen(void);    /* ROM 0x0AD5 */
extern void hu_basic_main(void);        /* ROM 0x1418 */

/* Out / In primitives */
extern void out_port(uint16_t port, uint8_t val);
extern uint8_t in_port(uint16_t port);
extern void enable_interrupts(void);
extern void set_interrupt_mode(int mode);

/**
 * 0x0062: Set VDG Display Mode
 */
void spc_set_gmode(uint8_t mode) {
    out_port(PORT_VDG_GMODE, mode);
    VAR_VDG_GMODE = mode;
}

/**
 * 0x006B: Set Cassette Motor & Control
 */
void spc_set_cassette(uint8_t val) {
    out_port(PORT_CASSETTE, val);
    VAR_CAS_MOTOR = val;
}

/**
 * 0x0056: Hardware Device Reset Routine
 */
void spc_init_hardware(void) {
    spc_set_gmode(0x00);      /* Set text display mode */
    spc_set_cassette(0x00);   /* Turn off cassette motor */
    vram_clear_screen();      /* Clear 6KB VRAM & character attributes */
}

/**
 * 0x004A: Primary System Startup Entry Point
 * (Executed when no floppy drive is attached or after ROM vector)
 */
void spc_system_bootstrap(void) {
    /* 0x004A: LD SP, 0x0000 (Wraps around to top of 64KB RAM 0xFFFF) */
    /* 0x004D: IM 1 */
    set_interrupt_mode(1);
    
    /* 0x004F: EI */
    enable_interrupts();
    
    /* 0x0050: CALL 0056h */
    spc_init_hardware();
    
    /* 0x0053: JP 1418h - Enter Hu-BASIC Interpreter Core */
    hu_basic_main();
}

/**
 * 0x0074: Vertical Blanking (60Hz) Timer Interrupt Handler (IM 1, Vector 0x0038)
 */
void spc_vblank_interrupt_handler(void) {
    VAR_TIMER_TICK++;
}
