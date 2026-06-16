// tests/test_cpu.cpp
#include <cassert>
#include <cstdio>
#include "chip8.h"

void test_reset_pc() {
    Chip8 c;
    c.reset();
    assert(c.program_counter == 0x200);
    printf("PASS  program_counter == 0x200\n");
}

void test_reset_registers_zero() {
    Chip8 c;
    c.registers[3] = 0xFF;  // dirty it first
    c.reset();
    for (int i = 0; i < 16; i++)
        assert(c.registers[i] == 0);
    printf("PASS  all registers == 0 after reset\n");
}

void test_reset_stack_zero() {
    Chip8 c;
    c.stack[5] = 0xAB;
    c.reset();
    for (int i = 0; i < 16; i++)
        assert(c.stack[i] == 0);
    printf("PASS  stack cleared\n");
}

void test_reset_sp_zero() {
    Chip8 c;
    c.stack_pointer = 7;
    c.reset();
    assert(c.stack_pointer == 0);
    printf("PASS  stack_pointer == 0\n");
}

void test_fontset_loaded() {
    Chip8 c;
    c.reset();
    // '0' sprite starts at 0x50, first byte is 0xF0
    assert(c.memory[0x50] == 0xF0);
    // '1' sprite starts at 0x55, first byte is 0x20
    assert(c.memory[0x55] == 0x20);
    // last byte of 'F' sprite
    assert(c.memory[0x9F] == 0x80);
    printf("PASS  fontset loaded at 0x50\n");
}

void test_memory_below_rom_is_zero() {
    Chip8 c;
    c.reset();
    // 0x000–0x04F should be zero (before fontset)
    for (int i = 0; i < 0x50; i++)
        assert(c.memory[i] == 0);
    printf("PASS  memory[0x000–0x04F] == 0\n");
}

int main() {
    test_reset_pc();
    test_reset_registers_zero();
    test_reset_stack_zero();
    test_reset_sp_zero();
    test_fontset_loaded();
    test_memory_below_rom_is_zero();
    printf("\nAll CPU tests passed.\n");
    return 0;
}
