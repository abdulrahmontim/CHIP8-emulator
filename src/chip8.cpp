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
        if (Y == 0xE) {
            for (int i = 0; i < (32 * 64); ++i) {
                display[i] = 0x0;
            }
        }
        break;    

    case 0x1:
        program_counter = NNN;
        break;

    case 0x6:
        // uint8_t second_nib = X;
        registers[X] = NN;
        break;

    case 0x7:
        registers[X] += NN;
        break;

    case 0xA:
        index_register = NNN;
        break;

    case 0xD:
        uint8_t start_X = registers[X];
        uint8_t start_Y = registers[Y];

        start_X %= 64;
        start_Y %= 32;

        registers[0xF] = 0;

        // uint8_t spriteBytes =  
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
