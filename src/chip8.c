#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Instruction helpers
void unrecognisedOpcode(uint16_t opcode)
{
    fprintf(stderr, "Opcode 0x%04x unrecognised\n", opcode);
    // while (true) { }
    exit(EXIT_FAILURE);
}

void jump(struct chip8* chip8, uint16_t NNN) { chip8->pc = NNN; }

void push(struct chip8* chip8)
{
    if (chip8->sp > CHIP8_STACK_SIZE - 1) {
        fprintf(stderr, "Stack Overflow\n");
        exit(EXIT_FAILURE);
    }
    // Push pc to stack at sp then increment pointer
    chip8->stack[chip8->sp++] = chip8->pc;
}

void pop(struct chip8* chip8)
{
    if (chip8->sp == 0) {
        return;
    }

    // Decrement sp then jump to address
    jump(chip8, chip8->stack[--chip8->sp]);
}

void convert_binary_to_decimal_and_load(struct chip8* chip8)
{
    uint8_t value = chip8->V[(chip8->opcode & 0x0F00) >> 8];

    uint8_t ones = value % 10;
    uint8_t tens = value / 10 % 10;
    uint8_t hundreds = value / 100 % 10;

    chip8->memory[chip8->index + 0] = hundreds;
    chip8->memory[chip8->index + 1] = tens;
    chip8->memory[chip8->index + 2] = ones;
}

// Instructions
void nop(struct chip8* chip8)
{
    unrecognisedOpcode(chip8->opcode);
}

void x0(struct chip8* chip8)
{

    switch (chip8->opcode) {
    case 0x00E0: // 00E0 - Clear screen
        memset(chip8->display, 0, sizeof(chip8->display));
        break;
    case 0x00EE: // 00EE - Return from subroutine
        pop(chip8);
        break;
    default:
        unrecognisedOpcode(chip8->opcode);
        break;
    }
}

void x1(struct chip8* chip8)
{
    // 1NNN - Jump to address NNN
    jump(chip8, chip8->opcode & 0x0FFF);
}

void x2(struct chip8* chip8)
{
    // 2NNN - Call subroutine at NNN (push pc to stack and jump)
    push(chip8);
    jump(chip8, chip8->opcode & 0x0FFF);
}

void x3(struct chip8* chip8)
{
    // 3XNN - Skip next instruction if VX = NN
    uint8_t VX = chip8->V[(chip8->opcode & 0x0F00) >> 8];
    uint16_t NN = chip8->opcode & 0xFF;

    if (VX == NN) {
        chip8->pc += 2;
    }
}

void x4(struct chip8* chip8)
{
    // 4XNN - Skip next instruction if VX != NN
    uint8_t VX = chip8->V[(chip8->opcode & 0x0F00) >> 8];
    uint16_t NN = chip8->opcode & 0xFF;

    if (VX != NN) {
        chip8->pc += 2;
    }
}

void x5(struct chip8* chip8)
{
    // 5XY0 - Skip next instruction if VX = VY
    uint8_t VX = chip8->V[(chip8->opcode & 0x0F00) >> 8];
    uint8_t VY = chip8->V[(chip8->opcode & 0x00F0) >> 8];

    if (VX == VY) {
        chip8->pc += 2;
    }
}

void x6(struct chip8* chip8)
{
    // 6XNN - Set register VX to value NN
    chip8->V[((chip8->opcode & 0x0F00) >> 8)] = (chip8->opcode & 0x00FF);
}

void x7(struct chip8* chip8)
{
    // 7XNN - Add value NN to register VX
    chip8->V[((chip8->opcode & 0x0F00) >> 8)] += (chip8->opcode & 0x00FF);
}

void x8(struct chip8* chip8)
{
    uint8_t code = chip8->opcode & 0x000F;
    uint8_t* VX = &chip8->V[(chip8->opcode & 0x0F00) >> 8];
    uint8_t VY = chip8->V[(chip8->opcode & 0x00F0) >> 4];
    uint8_t dropped_bit = 0;
    bool x8ShiftQuirk = false;
    bool underflow = false;
    bool overflow = false;

    switch (code) {
    // 8XY0 - Set VX to VY
    case 0x0:
        *VX = VY;
        break;
    // 8XY1 - Place the result of a bitwise OR on VX and VY into VX
    case 0x1:
        *VX |= VY;
        break;
    // 8XY2 - Place the result of a bitwise AND on VX and VY into VX
    case 0x2:
        *VX &= VY;
        break;
    // 8XY3 - Place the result of a bitwise XOR on VX and VY into VX
    case 0x3:
        *VX ^= VY;
        break;
    // 8XY4 - Add VX and VY and put it in VX. Set VF if it overflows
    case 0x4:
        overflow = (*VX + VY) > 0xFF;
        *VX += VY;
        chip8->V[0xF] = overflow;
        break;
    // 8XY5 - Subtract VY from VX and put it in VX. Set VF to 1 unless it
    // underflows in which case set it to 0
    case 0x5:
        underflow = VY > *VX;
        *VX -= VY;
        chip8->V[0xF] = (underflow ? 0x0 : 0x1);
        break;
    // 8XY6 - Shift VX one bit right. Set VX to VY if using quirk
    case 0x6:
        if (x8ShiftQuirk) {
            *VX = VY;
        }
        dropped_bit = (*VX & 0x1);
        *VX >>= 1;
        chip8->V[0xF] = dropped_bit;
        break;
    // 8XY7 - Subtract VX from VY and put it in VX. Set VF to 1 unless it
    // underflows in which case set it to 0
    case 0x7:
        underflow = *VX > VY;
        *VX = VY - *VX;
        chip8->V[0xF] = (underflow ? 0x0 : 0x1);
        break;
    // 8XYE - Shift VX one bit left. Set VX to VY if using quirk
    case 0xE:
        if (x8ShiftQuirk) {
            *VX = VY;
        }
        dropped_bit = (*VX >> 7);
        *VX <<= 1;
        chip8->V[0xF] = dropped_bit;
        break;
    default:
        unrecognisedOpcode(chip8->opcode);
        break;
    }
}

void x9(struct chip8* chip8)
{
    // 9XY0 - Skip next instruction if VX != VY
    uint8_t VX = chip8->V[(chip8->opcode & 0x0F00) >> 8];
    uint8_t VY = chip8->V[(chip8->opcode & 0x00F0) >> 4];

    if (VX != VY) {
        chip8->pc += 2;
    }
}

void xA(struct chip8* chip8)
{
    // ANNN - Set Index register to address NNN
    chip8->index = (chip8->opcode & 0x0FFF);
}

void xB(struct chip8* chip8)
{
    bool xBJumpQuirk = false;

    // BNNN - Jumps to address NNN + V0. If the quirx is enabled it instead
    // interprets it as XNN and jumps to address XNN + VX
    uint16_t NNN = chip8->opcode & 0x0FFF;

    if (xBJumpQuirk) {
        NNN += chip8->V[(chip8->opcode & 0x0F00) >> 8];
    } else {
        NNN += chip8->V[0x0];
    }
    jump(chip8, NNN);
}

void xC(struct chip8* chip8)
{
    // CXNN - Set VX to the result of a random number ANDed with NN
    uint8_t NN = chip8->opcode & 0x00FF;
    uint8_t* VX = &chip8->V[(chip8->opcode & 0x0F00) >> 8];

    uint8_t random = rand() & NN;

    *VX = random;
}

void xE(struct chip8* chip8)
{
    uint8_t* VX = &chip8->V[(chip8->opcode & 0x0F00) >> 8];
    bool isKeyPressed = chip8->keypad[*VX];

    switch (chip8->opcode & 0x00FF) {
    // EX9E - Skip next instruction if key VX is pressed
    case 0x9E:
        if (isKeyPressed) {
            chip8->pc += 2;
        }
        break;
    // EXA1 - Skip next instruction if key VX isn't pressed
    case 0xA1:
        if (!isKeyPressed) {
            chip8->pc += 2;
        }
        break;
    default:
        unrecognisedOpcode(chip8->opcode);
        break;
    }
}

void xD(struct chip8* chip8)
{
    // DXYN - Draw a sprite N pixels tall from the index register at position
    // VX, VY
    uint8_t X = chip8->V[(chip8->opcode & 0x0F00) >> 8] % 64;
    uint8_t Y = chip8->V[(chip8->opcode & 0x00F0) >> 4] % 32;
    uint8_t N = chip8->opcode & 0x000F;

    // Set VF to 0 to reset flag
    chip8->V[0xF] = 0;

    for (int row = 0; row < N; row++) {
        uint8_t pixelByte = chip8->memory[chip8->index + row];

        // Break if we reach the bottom of the screen
        if (Y + row > 31) {
            break;
        }

        for (int col = 0; col < 8; col++) {
            // Break if we reach the right edge of the screen
            if (X + col > 63) {
                break;
            }

            // Read the bit corresponding to the current position in the
            // sprite we're drawing
            bool pixel = (pixelByte & (0x80 >> col)) >> (7 - col);
            bool* displayPixel = &chip8->display[(X + col) + ((Y + row) * 64)];
            // If bit is already being displayed then turn it on and set flag
            if (pixel && *displayPixel) {
                *displayPixel = false;
                chip8->V[0xF] = 1;
                // If bit is not being displayed, turn it on
            } else if (pixel && !*displayPixel) {
                *displayPixel = true;
            }
        }
    }
}

void xF(struct chip8* chip8)
{
    uint8_t code = chip8->opcode & 0x00FF;
    uint8_t X = (chip8->opcode & 0x0F00) >> 8;
    uint8_t* VX = &chip8->V[X];
    uint16_t initialIndex = chip8->index;
    bool xFIndexQuirk = false;

    bool found = false;
    switch (code) {
    // FX07 - Set VX to the contents of the delay timer
    case 0x07:
        *VX = chip8->delay;
        break;
    // FX0A - Get pressed key and put it in VX
    case 0x0A:
        found = false;
        for (int i = 0; i < 0x10; i++) {
            if (chip8->keypad[i]) {
                *VX = i;
                found = true;
                break;
            }
        }
        if (!found) {
            chip8->pc -= 2;
        }
        break;
    // FX15 - Set the sound timer to the value in VX
    case 0x15:
        chip8->delay = *VX;
        break;
    // FX18 - Set the sound timer to the value in VX
    case 0x18:
        chip8->sound = *VX;
        break;
    // FX1E - Add VX to index register
    case 0x1E:
        chip8->index += *VX;
        // Set VF if index overflows addressable range
        chip8->V[0xF] = chip8->index >= 0x1000;
        break;
    // FX29 - Point index to font in memory for VX
    case 0x29:
        chip8->index = (0 + (*VX * 5));
        break;
    // FX33 - Convert VX to a binary number and store each digit in memory
    case 0x33:
        convert_binary_to_decimal_and_load(chip8);
        break;
    // FX55 - Store variables up to VX in successive memory locations
    case 0x55:
        for (int i = 0; i <= X; i++) {
            chip8->memory[initialIndex + i] = chip8->V[i];
            if (xFIndexQuirk) {
                chip8->index++;
            }
        }
        break;
    // FX65 - Load registers up to VX from successive memory locations
    case 0x65:
        for (int i = 0; i <= X; i++) {
            chip8->V[i] = chip8->memory[initialIndex + i];
            if (xFIndexQuirk) {
                chip8->index++;
            }
        }
        break;

    default:
        unrecognisedOpcode(chip8->opcode);
        break;
    }
}

static void (*OPS[0x10])(struct chip8*) = {
    x0, x1, x2, x3, //
    x4, x5, x6, x7, //
    x8, x9, xA, xB, //
    xC, xD, xE, xF, //
};

void chip8_initialize(struct chip8* chip8)
{
    // Ensure memory is clear
    memset(chip8, 0, sizeof(*chip8));

    // Load font data into the start of memory
    memcpy(chip8->memory, &FONT_DATA, sizeof(FONT_DATA));

    // Start program counter at 0x200 for compatibility
    chip8->pc = 0x200;
}

bool chip8_load_ROM(struct chip8* chip8, char* path)
{
    FILE* rom_file = fopen(path, "rb");
    if (rom_file == NULL) {
        fprintf(stderr, "Could not open %s", path);
        return false;
    }

    fseek(rom_file, 0, SEEK_END);
    long romFileSize = ftell(rom_file);
    rewind(rom_file);

    if (romFileSize == -1) {
        return false;
    }

    if (romFileSize > CHIP8_MEMORY_SIZE - 512) {
        fprintf(stderr, "The size of the specified ROM is larger than the available memory");
        return false;
    }

    // Read contents of the file into memory starting at 0x200
    fread(&chip8->memory[0x200], romFileSize, 1, rom_file);

    fclose(rom_file);
    return true;
}

void chip8_clock(struct chip8* chip8)
{
    // Fetch
    chip8->opcode = chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc + 1];
    chip8->pc += 2;

    // Decode
    uint8_t importantNibble = (chip8->opcode & 0xF000) >> 12;

    // Execute
    OPS[importantNibble](chip8);

    // Timers
    if (chip8->delay > 0) {
        chip8->delay--;
    }

    if (chip8->sound > 0) {
        chip8->sound--;
    }
}
