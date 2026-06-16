#include <iostream>
#include <fstream>
#include <vector>
#include "chip8.h"

using namespace std;


int MAX_PROGRAM_SIZE = 4096 - 0x200;

static const uint8_t fontset[80] = {
    0xF0,0x90,0x90,0x90,0xF0, // 0
    0x20,0x60,0x20,0x20,0x70, // 1
    0xF0,0x10,0xF0,0x80,0xF0, // 2
    0xF0,0x10,0xF0,0x10,0xF0, // 3
    0x90,0x90,0xF0,0x10,0x10, // 4
    0xF0,0x80,0xF0,0x10,0xF0, // 5
    0xF0,0x80,0xF0,0x90,0xF0, // 6
    0xF0,0x10,0x20,0x40,0x40, // 7
    0xF0,0x90,0xF0,0x90,0xF0, // 8
    0xF0,0x90,0xF0,0x10,0xF0, // 9
    0xF0,0x90,0xF0,0x90,0x90, // A
    0xE0,0x90,0xE0,0x90,0xE0, // B
    0xF0,0x80,0x80,0x80,0xF0, // C
    0xE0,0x90,0x90,0x90,0xE0, // D
    0xF0,0x80,0xF0,0x80,0xF0, // E
    0xF0,0x80,0xF0,0x80,0x80  // F
};

// 0x000 - 0x1FF interpreter
// 0x050 - 0x09F fontset
// 0x200 - 0xFFF ROM

void Chip8::loadFontset() {
    for (int i = 0; i < 80; i++) {
        memory[0x50 + i] = fontset[i];
    }
}

void Chip8::reset() {
    for (int i = 0; i < 4096; i++) memory[i] = 0;
    for (int i = 0; i < 16; i++)   registers[i] = 0;
    for (int i = 0; i < 16; i++)   stack[i] = 0;

    index_register  = 0;
    stack_pointer   = 0;
    program_counter = 0x200;
    delay_timer     = 0;
    sound_timer     = 0;

    loadFontset();
}

// 0x200 - 0xFFF ROM max: 0xDFF
bool Chip8::loadROM(const char* filename) {

    // reset();
    ifstream file(filename, ios::binary | std::ios::ate);

    if (!file.is_open()) return false;

    streampos size = file.tellg();

    if (size > MAX_PROGRAM_SIZE) {
        std::cerr << "Error: ROM size exceeds available memory" << endl;
        return false;
    }

    file.seekg(0, ios::beg);
    vector<char> buffer(size);

    if (file.read(buffer.data(), size)) {
        for (int i = 0; i < size; i++) {
            memory[0x200 + i] = buffer[i]; // 8 bits?
        }
    }

    file.close();
    return true;
}
