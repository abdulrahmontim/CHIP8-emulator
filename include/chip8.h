#pragma once

#include <stdint.h>


// mem reg cpu
struct Chip8 {
    uint8_t memory[4096] {};
    uint8_t registers[16] {};
    uint16_t index_register {};

    uint8_t stack[16] {};
    uint8_t stack_pointer {};
    uint16_t program_counter {};

    uint8_t delay_timer {};
    uint8_t sound_timer {};

    uint8_t keypad[16] {};
    uint8_t display[64 * 32] {};

    void loadFontset();
    bool loadROM(const char *filename);

    void emulateCycle();
    void debugPrintDisplay();


    void reset();
};


