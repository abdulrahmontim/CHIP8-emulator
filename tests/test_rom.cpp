// tests/test_rom.cpp
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include "chip8.h"

void create_test_rom(const char* filename, const uint8_t* data, int size) {
    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data), size);
    file.close();
}

void test_load_rom_valid() {
    // Create a small test ROM (12 bytes)
    uint8_t test_data[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11, 0x22, 0x33, 0x44};
    create_test_rom("test_rom.ch8", test_data, sizeof(test_data));

    Chip8 c;
    c.reset();
    
    bool result = c.loadROM("test_rom.ch8");
    assert(result == true);
    
    // Verify ROM is loaded at 0x200
    assert(c.memory[0x200] == 0x12);
    assert(c.memory[0x201] == 0x34);
    assert(c.memory[0x202] == 0x56);
    assert(c.memory[0x20B] == 0x44);  // last byte at 0x200 + 11
    
    printf("PASS  valid ROM loaded at 0x200\n");
    remove("test_rom.ch8");
}

void test_load_rom_nonexistent() {
    Chip8 c;
    c.reset();
    
    bool result = c.loadROM("nonexistent_rom.ch8");
    assert(result == false);
    
    printf("PASS  nonexistent ROM returns false\n");
}

void test_load_rom_preserves_fontset() {
    // Create a test ROM (10 bytes)
    uint8_t test_data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x12, 0x34, 0x56, 0x78};
    create_test_rom("test_rom2.ch8", test_data, sizeof(test_data));

    Chip8 c;
    c.reset();
    
    // Verify fontset is at 0x50 before ROM load
    uint8_t fontset_byte_before = c.memory[0x50];
    
    c.loadROM("test_rom2.ch8");
    
    // Verify fontset is still there after ROM load
    assert(c.memory[0x50] == fontset_byte_before);
    assert(c.memory[0x50] == 0xF0);  // First byte of '0' sprite
    
    printf("PASS  fontset preserved after ROM load\n");
    remove("test_rom2.ch8");
}

void test_load_rom_memory_boundary() {
    Chip8 c;
    c.reset();
    
    // Create a ROM that's near max size (3328 bytes = 4096 - 0x200)
    int max_rom_size = 4096 - 0x200;
    uint8_t* large_data = new uint8_t[max_rom_size];
    for (int i = 0; i < max_rom_size; i++) {
        large_data[i] = (i % 256);
    }
    
    create_test_rom("large_rom.ch8", large_data, max_rom_size);
    
    bool result = c.loadROM("large_rom.ch8");
    assert(result == true);
    
    // Verify data at start and end
    assert(c.memory[0x200] == 0);
    assert(c.memory[0xFFF] == (max_rom_size - 1) % 256);
    
    printf("PASS  max-size ROM loaded correctly\n");
    remove("large_rom.ch8");
    delete[] large_data;
}

int main() {
    test_load_rom_valid();
    test_load_rom_nonexistent();
    test_load_rom_preserves_fontset();
    test_load_rom_memory_boundary();
    printf("\nAll ROM tests passed.\n");
    return 0;
}
