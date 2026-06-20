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

// TODO: optimize rom copying
void Chip8::emulateCycle() {

    // fetch();
    // decode();
    // execute();
    // refactor into, if possible 
    
    uint16_t opcode = (memory[program_counter] << 8) | memory[program_counter + 1];
    program_counter += 2;

    uint8_t category = (opcode & 0xF000) >> 12;
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t Y = (opcode & 0x00F0) >> 4;
    uint8_t N = opcode & 0x000F;
    uint8_t NN = opcode & 0x00FF;
    uint16_t NNN = opcode & 0x0FFF;

   switch (category) {
    case 0x0:
        if (Y == 0xE && N == 0x0) {
            for (int i = 0; i < (32 * 64); ++i) {
                display[i] = 0x0;
            }
        } else if (Y == 0xE && N == 0xE) {
            if (stack_pointer > 0) {
                stack_pointer--;
                program_counter = stack[stack_pointer];
            }
        }
        break;    

    case 0x1:
        program_counter = NNN;
        break;
    
    case 0x2:
        stack[stack_pointer] = program_counter;
        stack_pointer++;
        program_counter = NNN;
        break;
    
    case 0x3:
        if (registers[X] == NN) program_counter += 2;
        break;
    
    case 0x4:
        if (registers[X] != NN) program_counter += 2;
        break;

    case 0x5:
        if (registers[X] == registers[Y]) program_counter += 2;
        break;
    
    case 0x6:
        registers[X] = NN;
        break;

    case 0x7:
        registers[X] += NN;
        break;
    
    case 0x8:
        if (N == 0x0) registers[X] = registers[Y];
        else if (N == 0x1) registers[X] |= registers[Y];
        else if (N == 0x2) registers[X] &= registers[Y];
        else if (N == 0x3) registers[X] ^= registers[Y];
        else if (N == 0x4) {
            uint16_t reg_sum = registers[X] + registers[Y];
            registers[0xF] = (reg_sum > 255) ? 1 : 0;
            registers[X] = (uint8_t)reg_sum;

        } else if (N == 0x5) {
            registers[0xF] = (registers[X] >= registers[Y]) ? 1 : 0;
            registers[X] = registers[X] - registers[Y];

        } else if (N == 0x6) {
            uint8_t lsb = registers[X] & 0x01;
            registers[X] >>= 1;
            registers[0xF] = lsb;

        } else if (N == 0x7) {
            registers[0xF] = (registers[Y] > registers[X]) ? 1 : 0;
            registers[X] = registers[Y] - registers[X];
        }
        else if (N == 0xE) {
            uint8_t msb = registers[X] >> 7;
            registers[0xF] = msb;
            registers[X] <<= 1;
        }

        break;
    
    case 0x9:
        if (registers[X] != registers[Y]) program_counter += 2;
        break;

    case 0xA:
        index_register = NNN;
        break;
    
    case 0xB:
        program_counter = NNN + registers[0];
        break;
    
    case 0xC: {
        uint8_t random_byte = rand() % 256;
        registers[X] = random_byte & NN;
        break;
    }

    case 0xD: {
        uint8_t start_X = registers[X];
        uint8_t start_Y = registers[Y];

        start_X %= 64;
        start_Y %= 32;

        registers[0xF] = 0;

        for (int row = 0; row < N; ++row) {
            uint8_t sprite_byte = memory[index_register + row];

            for (int col = 0; col < 8; ++col) {
                uint8_t sprite_pixel = sprite_byte & (0x80 >> col);

                if (sprite_pixel) {
                    int display_X = (start_X + col) % 64;
                    int display_Y = (start_Y + row) % 32;
                    int screen_index = (display_Y * 64) + display_X;

                    if (display[screen_index]) registers[0xF] = 1;
                    display[screen_index] ^= 1;
                }

            }
        }
        break;
    }

    case 0xE:
        if (NN == 0x9E) {
            if (keypad[registers[X]])
                program_counter += 2;
        } else if (NN == 0xA1) {
            if (!keypad[registers[X]])
                program_counter += 2;
        }
        break;

    case 0xF:
            if (NN == 0x07) registers[X] = delay_timer;
            else if (NN == 0x0A) {
                bool key_pressed = false;
                for (int i = 0; i < 16; i++) {
                    if (keypad[i]) {
                        registers[X] = i;
                        key_pressed = true;
                        break;
                    }
                }
                if (!key_pressed) program_counter -= 2;
            }
            else if (NN == 0x15) delay_timer = registers[X];
            else if (NN == 0x18) sound_timer = registers[X];
            else if (NN == 0x1E) index_register += registers[X];
            else if (NN == 0x29) index_register = 0x50 + ((registers[X] & 0x0F) * 5);

            else if (NN == 0x33) {
                uint8_t value = registers[X];
                memory[index_register] = value / 100;
                memory[index_register + 1] = (value / 10) % 10;
                memory[index_register + 2] = value % 10;
            }

            else if (NN == 0x55) {
                for (int i = 0; i <= X; i++) {
                    memory[index_register + i] = registers[i];
                }
            }

            else if (NN == 0x65) {
                for (int i = 0; i <= X; i++) {
                    registers[i] = memory[index_register + i];
                }
            }
        break;

    }

}


void Chip8::debugPrintDisplay() {
    for (int i = 0; i < 64 * 32; i++) {
        cout << (display[i] ? "#" : " ");

        if ((i + 1) % 64 == 0) {
            cout << "\n";
        }
    }
    cout << "\n";
}
