#ifndef CHIP8_H
#define CHIP8_H

#include <stdbool.h>
#include <stdint.h>

#define CHIP8_STACK_SIZE 16
#define CHIP8_MEMORY_SIZE 4096

static uint8_t const FONT_DATA[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};

struct chip8 {
    uint16_t opcode;                   // Current instruction
    uint16_t index;                    // Index register
    uint16_t pc;                       // Program counter
    uint16_t stack[CHIP8_STACK_SIZE];  // Stack of addresses for returning from calls
    uint8_t sound;                     // Sound timer
    uint8_t delay;                     // Delay timer
    uint8_t sp;                        // Stack pointer
    uint8_t V[16];                     // Registers 0-15
    bool keypad[16];                   // 16 digit keypad
    uint8_t memory[CHIP8_MEMORY_SIZE]; // 4kB RAM
    bool display[64 * 32];             // Pixel array for display
};

void chip8_initialize(struct chip8*);
bool chip8_load_ROM(struct chip8*, char* path);
void chip8_clock(struct chip8*);

#endif
