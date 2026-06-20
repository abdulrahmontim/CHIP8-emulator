// tests/test_opcodes.cpp
#include <cassert>
#include <cstdio>
#include <cstring>
#include <setjmp.h>
#include "chip8.h"

// Non-aborting assert helper for TDD: tracks pass/fail without aborting.
static jmp_buf test_jmp;
static int test_failures = 0;

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL  %s (line %d)\n", __func__, __LINE__); \
        test_failures++; \
        longjmp(test_jmp, 1); \
    } \
} while(0)

#define RUN_TEST(name) do { \
    if (setjmp(test_jmp) == 0) { \
        name(); \
    } \
} while(0)

static void place_opcode(Chip8 &c, uint16_t addr, uint16_t opcode) {
    c.memory[addr]     = (opcode >> 8) & 0xFF;
    c.memory[addr + 1] = opcode & 0xFF;
}

// ── Fetch/Flow ──────────────────────────────────────────────────

void test_fetch_increments_pc() {
    Chip8 c;
    c.reset();
    place_opcode(c, 0x200, 0x6001);
    uint16_t old_pc = c.program_counter;
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == old_pc + 2);
    printf("PASS  PC incremented by 2 after fetch\n");
}

void test_sequential_opcodes() {
    Chip8 c;
    c.reset();
    place_opcode(c, 0x200, 0x6010);
    place_opcode(c, 0x202, 0x6120);
    place_opcode(c, 0x204, 0x7010);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0x10);
    TEST_ASSERT(c.program_counter == 0x202);
    c.emulateCycle();
    TEST_ASSERT(c.registers[1] == 0x20);
    TEST_ASSERT(c.program_counter == 0x204);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0x20);
    TEST_ASSERT(c.program_counter == 0x206);
    printf("PASS  sequential opcodes execute in order\n");
}

// ── 0NNN ────────────────────────────────────────────────────────

void test_opcode_00e0_clears_display() {
    Chip8 c;
    c.reset();
    c.display[0] = 1;
    c.display[100] = 1;
    c.display[2047] = 1;
    place_opcode(c, 0x200, 0x00E0);
    c.emulateCycle();
    for (int i = 0; i < 64 * 32; i++)
        TEST_ASSERT(c.display[i] == 0);
    printf("PASS  00E0 clears display\n");
}

void test_opcode_00ee_return() {
    Chip8 c;
    c.reset();
    c.stack[0] = 0x50;
    c.stack_pointer = 1;
    place_opcode(c, 0x200, 0x00EE);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x50);
    TEST_ASSERT(c.stack_pointer == 0);
    printf("PASS  00EE return from subroutine\n");
}

void test_opcode_00ee_return_underflow() {
    Chip8 c;
    c.reset();
    c.stack_pointer = 0;
    place_opcode(c, 0x200, 0x00EE);
    c.emulateCycle();
    TEST_ASSERT(c.stack_pointer == 0);
    printf("PASS  00EE return with empty stack does not underflow\n");
}

// ── 1NNN ────────────────────────────────────────────────────────

void test_opcode_1nnn_jump() {
    Chip8 c;
    c.reset();
    place_opcode(c, 0x200, 0x1ABC);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x0ABC);
    printf("PASS  1NNN jump to 0x%03X\n", 0xABC);
}

void test_opcode_1nnn_jump_to_zero() {
    Chip8 c;
    c.reset();
    place_opcode(c, 0x200, 0x1000);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x0000);
    printf("PASS  1NNN jump to 0x000\n");
}

// ── 2NNN ────────────────────────────────────────────────────────

void test_opcode_2nnn_call() {
    Chip8 c;
    c.reset();
    place_opcode(c, 0x200, 0x2ABC);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x0ABC);
    TEST_ASSERT(c.stack_pointer == 1);
    printf("PASS  2NNN call subroutine pushes return address\n");
}

void test_opcode_2nnn_call_nested() {
    Chip8 c;
    c.reset();
    place_opcode(c, 0x200, 0x2222);
    place_opcode(c, 0x222, 0x2333);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x222);
    TEST_ASSERT(c.stack_pointer == 1);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x333);
    TEST_ASSERT(c.stack_pointer == 2);
    printf("PASS  2NNN nested calls\n");
}

void test_opcode_2nnn_call_stack_overflow() {
    Chip8 c;
    c.reset();
    for (int i = 0; i < 20; i++)
        place_opcode(c, 0x200 + i * 2, 0x2202 + i * 2);
    for (int i = 0; i < 16; i++)
        c.emulateCycle();
    TEST_ASSERT(c.stack_pointer == 16);
    printf("PASS  2NNN call stack saturates at 16 entries\n");
}

// ── 3XNN ────────────────────────────────────────────────────────

void test_opcode_3xnn_skip_equal() {
    Chip8 c;
    c.reset();
    c.registers[3] = 0x42;
    place_opcode(c, 0x200, 0x3342);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x204);
    printf("PASS  3XNN skips when VX == NN\n");
}

void test_opcode_3xnn_no_skip_not_equal() {
    Chip8 c;
    c.reset();
    c.registers[3] = 0x41;
    place_opcode(c, 0x200, 0x3342);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x202);
    printf("PASS  3XNN does not skip when VX != NN\n");
}

// ── 4XNN ────────────────────────────────────────────────────────

void test_opcode_4xnn_skip_not_equal() {
    Chip8 c;
    c.reset();
    c.registers[4] = 0x10;
    place_opcode(c, 0x200, 0x4442);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x204);
    printf("PASS  4XNN skips when VX != NN\n");
}

void test_opcode_4xnn_no_skip_equal() {
    Chip8 c;
    c.reset();
    c.registers[4] = 0x42;
    place_opcode(c, 0x200, 0x4442);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x202);
    printf("PASS  4XNN does not skip when VX == NN\n");
}

// ── 5XY0 ────────────────────────────────────────────────────────

void test_opcode_5xy0_skip_equal() {
    Chip8 c;
    c.reset();
    c.registers[2] = 0x55;
    c.registers[3] = 0x55;
    place_opcode(c, 0x200, 0x5230);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x204);
    printf("PASS  5XY0 skips when VX == VY\n");
}

void test_opcode_5xy0_no_skip_not_equal() {
    Chip8 c;
    c.reset();
    c.registers[2] = 0x55;
    c.registers[3] = 0x56;
    place_opcode(c, 0x200, 0x5230);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x202);
    printf("PASS  5XY0 does not skip when VX != VY\n");
}

// ── 6XNN ────────────────────────────────────────────────────────

void test_opcode_6xnn_set_register() {
    Chip8 c;
    c.reset();
    place_opcode(c, 0x200, 0x6A42);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0xA] == 0x42);
    printf("PASS  6XNN set register V%X = 0x%02X\n", 0xA, 0x42);
}

void test_opcode_6xnn_set_register_zero() {
    Chip8 c;
    c.reset();
    c.registers[7] = 0xFF;
    place_opcode(c, 0x200, 0x6700);
    c.emulateCycle();
    TEST_ASSERT(c.registers[7] == 0x00);
    printf("PASS  6XNN set register to 0\n");
}

// ── 7XNN ────────────────────────────────────────────────────────

void test_opcode_7xnn_add() {
    Chip8 c;
    c.reset();
    c.registers[3] = 0x10;
    place_opcode(c, 0x200, 0x7320);
    c.emulateCycle();
    TEST_ASSERT(c.registers[3] == 0x30);
    printf("PASS  7XNN add: 0x10 + 0x20 = 0x30\n");
}

void test_opcode_7xnn_add_wraps() {
    Chip8 c;
    c.reset();
    c.registers[5] = 0xFF;
    place_opcode(c, 0x200, 0x7501);
    c.emulateCycle();
    TEST_ASSERT(c.registers[5] == 0x00);
    printf("PASS  7XNN add wraps: 0xFF + 1 = 0x00\n");
}

// ── 8XY0-8XYE ───────────────────────────────────────────────────

void test_opcode_8xy0_ld() {
    Chip8 c;
    c.reset();
    c.registers[1] = 0xAB;
    place_opcode(c, 0x200, 0x8010);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0xAB);
    printf("PASS  8XY0 LD VX, VY: V0 = V1\n");
}

void test_opcode_8xy1_or() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0xF0;
    c.registers[1] = 0x0F;
    place_opcode(c, 0x200, 0x8011);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0xFF);
    printf("PASS  8XY1 OR VX, VY: 0xF0 | 0x0F = 0xFF\n");
}

void test_opcode_8xy2_and() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0xFF;
    c.registers[1] = 0x0F;
    place_opcode(c, 0x200, 0x8012);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0x0F);
    printf("PASS  8XY2 AND VX, VY: 0xFF & 0x0F = 0x0F\n");
}

void test_opcode_8xy3_xor() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0xFF;
    c.registers[1] = 0x0F;
    place_opcode(c, 0x200, 0x8013);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0xF0);
    printf("PASS  8XY3 XOR VX, VY: 0xFF ^ 0x0F = 0xF0\n");
}

void test_opcode_8xy4_add_no_carry() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x10;
    c.registers[1] = 0x20;
    place_opcode(c, 0x200, 0x8014);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0x30);
    TEST_ASSERT(c.registers[0xF] == 0);
    printf("PASS  8XY4 ADD no carry: 0x10 + 0x20 = 0x30, VF=0\n");
}

void test_opcode_8xy4_add_with_carry() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0xFF;
    c.registers[1] = 0x01;
    place_opcode(c, 0x200, 0x8014);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0x00);
    TEST_ASSERT(c.registers[0xF] == 1);
    printf("PASS  8XY4 ADD with carry: 0xFF + 0x01 = 0x00, VF=1\n");
}

void test_opcode_8xy4_add_carry_exact_256() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x80;
    c.registers[1] = 0x80;
    place_opcode(c, 0x200, 0x8014);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0x00);
    TEST_ASSERT(c.registers[0xF] == 1);
    printf("PASS  8XY4 ADD carry: 0x80 + 0x80 = 0x00, VF=1\n");
}

void test_opcode_8xy5_sub_no_borrow() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x30;
    c.registers[1] = 0x10;
    place_opcode(c, 0x200, 0x8015);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0x20);
    TEST_ASSERT(c.registers[0xF] == 1);
    printf("PASS  8XY5 SUB no borrow: 0x30 - 0x10 = 0x20, VF=1\n");
}

void test_opcode_8xy5_sub_with_borrow() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x10;
    c.registers[1] = 0x30;
    place_opcode(c, 0x200, 0x8015);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0xE0);
    TEST_ASSERT(c.registers[0xF] == 0);
    printf("PASS  8XY5 SUB with borrow: 0x10 - 0x30 = 0xE0, VF=0\n");
}

void test_opcode_8xy5_sub_equal() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x50;
    c.registers[1] = 0x50;
    place_opcode(c, 0x200, 0x8015);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0x00);
    TEST_ASSERT(c.registers[0xF] == 1);
    printf("PASS  8XY5 SUB equal: 0x50 - 0x50 = 0x00, VF=1\n");
}

void test_opcode_8xy6_shr_lsb_zero() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x02;
    place_opcode(c, 0x200, 0x8016);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0x01);
    TEST_ASSERT(c.registers[0xF] == 0);
    printf("PASS  8XY6 SHR: 0x02 >> 1 = 0x01, VF=0\n");
}

void test_opcode_8xy6_shr_lsb_one() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x03;
    place_opcode(c, 0x200, 0x8016);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0x01);
    TEST_ASSERT(c.registers[0xF] == 1);
    printf("PASS  8XY6 SHR: 0x03 >> 1 = 0x01, VF=1 (LSB was 1)\n");
}

void test_opcode_8xy7_subn_no_borrow() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x10;
    c.registers[1] = 0x30;
    place_opcode(c, 0x200, 0x8017);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0x20);
    TEST_ASSERT(c.registers[0xF] == 1);
    printf("PASS  8XY7 SUBN: 0x30 - 0x10 = 0x20, VF=1\n");
}

void test_opcode_8xy7_subn_with_borrow() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x30;
    c.registers[1] = 0x10;
    place_opcode(c, 0x200, 0x8017);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0xE0);
    TEST_ASSERT(c.registers[0xF] == 0);
    printf("PASS  8XY7 SUBN with borrow: 0x10 - 0x30 = 0xE0, VF=0\n");
}

void test_opcode_8xye_shl_msb_zero() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x7F;
    place_opcode(c, 0x200, 0x801E);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0xFE);
    TEST_ASSERT(c.registers[0xF] == 0);
    printf("PASS  8XYE SHL: 0x7F << 1 = 0xFE, VF=0 (MSB was 0)\n");
}

void test_opcode_8xye_shl_msb_one() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0xFF;
    place_opcode(c, 0x200, 0x801E);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0xFE);
    TEST_ASSERT(c.registers[0xF] == 1);
    printf("PASS  8XYE SHL: 0xFF << 1 = 0xFE, VF=1 (MSB was 1)\n");
}

// ── 9XY0 ────────────────────────────────────────────────────────

void test_opcode_9xy0_skip_not_equal() {
    Chip8 c;
    c.reset();
    c.registers[2] = 0x55;
    c.registers[3] = 0x56;
    place_opcode(c, 0x200, 0x9230);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x204);
    printf("PASS  9XY0 skips when VX != VY\n");
}

void test_opcode_9xy0_no_skip_equal() {
    Chip8 c;
    c.reset();
    c.registers[2] = 0x55;
    c.registers[3] = 0x55;
    place_opcode(c, 0x200, 0x9230);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x202);
    printf("PASS  9XY0 does not skip when VX == VY\n");
}

// ── ANNN ────────────────────────────────────────────────────────

void test_opcode_annn_set_index() {
    Chip8 c;
    c.reset();
    place_opcode(c, 0x200, 0xAABC);
    c.emulateCycle();
    TEST_ASSERT(c.index_register == 0x0ABC);
    printf("PASS  ANNN set index = 0x%03X\n", 0xABC);
}

void test_opcode_annn_set_index_high() {
    Chip8 c;
    c.reset();
    place_opcode(c, 0x200, 0xAFFF);
    c.emulateCycle();
    TEST_ASSERT(c.index_register == 0x0FFF);
    printf("PASS  ANNN set index = 0x%03X (high address)\n", 0xFFF);
}

// ── BNNN ────────────────────────────────────────────────────────

void test_opcode_bnnn_jump_offset() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x50;
    place_opcode(c, 0x200, 0xB100);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x0150);
    printf("PASS  BNNN jump V0 + NNN: 0x50 + 0x100 = 0x150\n");
}

void test_opcode_bnnn_jump_offset_zero() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x00;
    place_opcode(c, 0x200, 0xBA00);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x0A00);
    printf("PASS  BNNN jump with V0=0: 0 + 0xA00 = 0xA00\n");
}

// ── CXNN ────────────────────────────────────────────────────────

void test_opcode_cxnn_random_masked() {
    Chip8 c;
    c.reset();
    // Run many times and verify result is always masked by NN
    bool seen_nonzero = false;
    bool seen_zero = false;
    for (int i = 0; i < 100; i++) {
        c.registers[0] = 0xFF; // reset
        place_opcode(c, 0x200, 0xC00F); // RND V0, 0x0F
        c.emulateCycle();
        TEST_ASSERT((c.registers[0] & 0xF0) == 0);
        if (c.registers[0] != 0) seen_nonzero = true;
        if (c.registers[0] == 0) seen_zero = true;
        c.program_counter = 0x200; // re-run
    }
    TEST_ASSERT(seen_nonzero && seen_zero);
    printf("PASS  CXNN random masked with 0x0F\n");
}

void test_opcode_cxnn_random_mask_zero() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0xFF;
    for (int i = 0; i < 10; i++) {
        place_opcode(c, 0x200, 0xC000); // RND V0, 0x00
        c.emulateCycle();
        TEST_ASSERT(c.registers[0] == 0);
        c.program_counter = 0x200;
    }
    printf("PASS  CXNN random masked with 0x00 always 0\n");
}

// ── DXYN ────────────────────────────────────────────────────────

void test_opcode_dxyn_draw_turns_pixels_on() {
    Chip8 c;
    c.reset();
    uint16_t sprite_addr = 0x300;
    c.memory[sprite_addr] = 0xF0;
    c.index_register = sprite_addr;
    c.registers[0] = 0;
    c.registers[1] = 0;
    place_opcode(c, 0x200, 0xD011);
    c.emulateCycle();
    TEST_ASSERT(c.display[0] == 1);
    TEST_ASSERT(c.display[1] == 1);
    TEST_ASSERT(c.display[2] == 1);
    TEST_ASSERT(c.display[3] == 1);
    TEST_ASSERT(c.display[4] == 0);
    TEST_ASSERT(c.display[5] == 0);
    TEST_ASSERT(c.display[6] == 0);
    TEST_ASSERT(c.display[7] == 0);
    TEST_ASSERT(c.registers[0xF] == 0);
    printf("PASS  DXYN draws sprite pixels correctly\n");
}

void test_opcode_dxyn_xor_collision() {
    Chip8 c;
    c.reset();
    uint16_t sprite_addr = 0x300;
    c.memory[sprite_addr] = 0x80;
    c.index_register = sprite_addr;
    c.registers[0] = 0;
    c.registers[1] = 0;
    place_opcode(c, 0x200, 0xD011);
    c.emulateCycle();
    TEST_ASSERT(c.display[0] == 1);
    TEST_ASSERT(c.registers[0xF] == 0);
    place_opcode(c, 0x202, 0xD011);
    c.emulateCycle();
    TEST_ASSERT(c.display[0] == 0);
    TEST_ASSERT(c.registers[0xF] == 1);
    printf("PASS  DXYN XOR collision sets VF=1\n");
}

void test_opcode_dxyn_no_collision_clears_vf() {
    Chip8 c;
    c.reset();
    uint16_t sprite_addr = 0x300;
    c.memory[sprite_addr] = 0x80;
    c.index_register = sprite_addr;
    c.registers[0] = 0;
    c.registers[1] = 0;
    c.registers[0xF] = 0xFF;
    place_opcode(c, 0x200, 0xD011);
    c.emulateCycle();
    TEST_ASSERT(c.display[0] == 1);
    TEST_ASSERT(c.registers[0xF] == 0);
    printf("PASS  DXYN no collision clears VF to 0\n");
}

void test_opcode_dxyn_wraps_x() {
    Chip8 c;
    c.reset();
    uint16_t sprite_addr = 0x300;
    c.memory[sprite_addr] = 0xC0;
    c.index_register = sprite_addr;
    c.registers[0] = 63;
    c.registers[1] = 0;
    place_opcode(c, 0x200, 0xD011);
    c.emulateCycle();
    TEST_ASSERT(c.display[63] == 1);
    TEST_ASSERT(c.display[0] == 1);
    printf("PASS  DXYN wraps X coordinate modulo 64\n");
}

void test_opcode_dxyn_wraps_y() {
    Chip8 c;
    c.reset();
    uint16_t sprite_addr = 0x300;
    c.memory[sprite_addr] = 0x80;
    c.index_register = sprite_addr;
    c.registers[0] = 0;
    c.registers[1] = 31;
    place_opcode(c, 0x200, 0xD011);
    c.emulateCycle();
    TEST_ASSERT(c.display[31 * 64] == 1);
    printf("PASS  DXYN wraps Y coordinate modulo 32\n");
}

void test_opcode_dxyn_multi_row() {
    Chip8 c;
    c.reset();
    uint16_t sprite_addr = 0x300;
    c.memory[sprite_addr]     = 0xF0;
    c.memory[sprite_addr + 1] = 0x90;
    c.index_register = sprite_addr;
    c.registers[0] = 0;
    c.registers[1] = 0;
    place_opcode(c, 0x200, 0xD012);
    c.emulateCycle();
    TEST_ASSERT(c.display[0] == 1);
    TEST_ASSERT(c.display[3] == 1);
    TEST_ASSERT(c.display[4] == 0);
    TEST_ASSERT(c.display[64 + 0] == 1);
    TEST_ASSERT(c.display[64 + 3] == 1);
    TEST_ASSERT(c.display[64 + 1] == 0);
    printf("PASS  DXYN draws multi-row sprite (N=2)\n");
}

void test_opcode_dxyn_vf_not_affected_when_sprite_byte_zero() {
    Chip8 c;
    c.reset();
    uint16_t sprite_addr = 0x300;
    c.memory[sprite_addr] = 0x00;
    c.index_register = sprite_addr;
    c.registers[0] = 0;
    c.registers[1] = 0;
    c.registers[0xF] = 0xFF;
    place_opcode(c, 0x200, 0xD011);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0xF] == 0);
    printf("PASS  DXYN VF cleared when sprite byte is zero\n");
}

// ── EX9E / EXA1 ─────────────────────────────────────────────────

void test_opcode_ex9e_skip_key_pressed() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x05;
    c.keypad[0x05] = 1;
    place_opcode(c, 0x200, 0xE09E);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x204);
    printf("PASS  EX9E skips when key VX is pressed\n");
}

void test_opcode_ex9e_no_skip_key_not_pressed() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x05;
    c.keypad[0x05] = 0;
    place_opcode(c, 0x200, 0xE09E);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x202);
    printf("PASS  EX9E does not skip when key VX is not pressed\n");
}

void test_opcode_exa1_skip_key_not_pressed() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x05;
    c.keypad[0x05] = 0;
    place_opcode(c, 0x200, 0xE0A1);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x204);
    printf("PASS  EXA1 skips when key VX is not pressed\n");
}

void test_opcode_exa1_no_skip_key_pressed() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x05;
    c.keypad[0x05] = 1;
    place_opcode(c, 0x200, 0xE0A1);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x202);
    printf("PASS  EXA1 does not skip when key VX is pressed\n");
}

// ── FX07 ────────────────────────────────────────────────────────

void test_opcode_fx07_ld_vx_dt() {
    Chip8 c;
    c.reset();
    c.delay_timer = 0x7B;
    place_opcode(c, 0x200, 0xF007);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0x7B);
    printf("PASS  FX07 LD VX, DT: V0 = 0x7B\n");
}

// ── FX0A ────────────────────────────────────────────────────────

void test_opcode_fx0a_wait_key_blocks() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x00;
    place_opcode(c, 0x200, 0xF00A);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x200);
    printf("PASS  FX0A wait key blocks (PC not incremented)\n");
}

void test_opcode_fx0a_wait_key_stores_key() {
    Chip8 c;
    c.reset();
    c.keypad[0x07] = 1;
    place_opcode(c, 0x200, 0xF00A);
    c.emulateCycle();
    TEST_ASSERT(c.registers[0] == 0x07);
    TEST_ASSERT(c.program_counter == 0x202);
    printf("PASS  FX0A wait key stores key code 0x07 in V0\n");
}

void test_opcode_fx0a_wait_key_picks_first_pressed() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0xFF;
    c.keypad[0x03] = 1;
    c.keypad[0x08] = 1;
    place_opcode(c, 0x200, 0xF00A);
    c.emulateCycle();
    TEST_ASSERT(c.program_counter == 0x202);
    TEST_ASSERT(c.registers[0] == 0x03 || c.registers[0] == 0x08);
    printf("PASS  FX0A wait key picks a pressed key\n");
}

// ── FX15 ────────────────────────────────────────────────────────

void test_opcode_fx15_ld_dt_vx() {
    Chip8 c;
    c.reset();
    c.registers[3] = 0x99;
    place_opcode(c, 0x200, 0xF315);
    c.emulateCycle();
    TEST_ASSERT(c.delay_timer == 0x99);
    printf("PASS  FX15 LD DT, VX: delay_timer = 0x99\n");
}

// ── FX18 ────────────────────────────────────────────────────────

void test_opcode_fx18_ld_st_vx() {
    Chip8 c;
    c.reset();
    c.registers[4] = 0x55;
    place_opcode(c, 0x200, 0xF418);
    c.emulateCycle();
    TEST_ASSERT(c.sound_timer == 0x55);
    printf("PASS  FX18 LD ST, VX: sound_timer = 0x55\n");
}

// ── FX1E ────────────────────────────────────────────────────────

void test_opcode_fx1e_add_i_vx() {
    Chip8 c;
    c.reset();
    c.index_register = 0x100;
    c.registers[2] = 0x050;
    place_opcode(c, 0x200, 0xF21E);
    c.emulateCycle();
    TEST_ASSERT(c.index_register == 0x0150);
    printf("PASS  FX1E ADD I, VX: 0x100 + 0x50 = 0x150\n");
}

void test_opcode_fx1e_add_i_vx_wrap() {
    Chip8 c;
    c.reset();
    c.index_register = 0xF00;
    c.registers[0] = 0xFF;
    place_opcode(c, 0x200, 0xF01E);
    c.emulateCycle();
    TEST_ASSERT(c.index_register == 0x0FFF);
    printf("PASS  FX1E ADD I, VX: 0xF00 + 0xFF = 0xFFF\n");
}

// ── FX29 ────────────────────────────────────────────────────────

void test_opcode_fx29_ld_font_sprite() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x00;
    place_opcode(c, 0x200, 0xF029);
    c.emulateCycle();
    TEST_ASSERT(c.index_register == 0x50);
    printf("PASS  FX29 LD F, V0: font sprite for '0' at 0x50\n");
}

void test_opcode_fx29_ld_font_sprite_digit_f() {
    Chip8 c;
    c.reset();
    c.registers[1] = 0x0F;
    place_opcode(c, 0x200, 0xF129);
    c.emulateCycle();
    TEST_ASSERT(c.index_register == 0x9B);
    printf("PASS  FX29 LD F, V1: font sprite for 'F' at 0x9B\n");
}

void test_opcode_fx29_ld_font_sprite_vx_beyond_f() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0x1F;
    place_opcode(c, 0x200, 0xF029);
    c.emulateCycle();
    TEST_ASSERT(c.index_register == 0x50 + (0x0F * 5));
    printf("PASS  FX29 LD F, VX masked to 0x0F: 0x1F -> 0x0F -> 0x%03X\n", 0x50 + (0x0F * 5));
}

// ── FX33 ────────────────────────────────────────────────────────

void test_opcode_fx33_bcd() {
    Chip8 c;
    c.reset();
    c.index_register = 0x300;
    c.registers[0] = 123;
    place_opcode(c, 0x200, 0xF033);
    c.emulateCycle();
    TEST_ASSERT(c.memory[0x300] == 1);
    TEST_ASSERT(c.memory[0x301] == 2);
    TEST_ASSERT(c.memory[0x302] == 3);
    printf("PASS  FX33 BCD: 123 -> [1, 2, 3] at I..I+2\n");
}

void test_opcode_fx33_bcd_zero() {
    Chip8 c;
    c.reset();
    c.index_register = 0x300;
    c.memory[0x300] = 0xFF;
    c.memory[0x301] = 0xFF;
    c.memory[0x302] = 0xFF;
    c.registers[0] = 0;
    place_opcode(c, 0x200, 0xF033);
    c.emulateCycle();
    TEST_ASSERT(c.memory[0x300] == 0);
    TEST_ASSERT(c.memory[0x301] == 0);
    TEST_ASSERT(c.memory[0x302] == 0);
    printf("PASS  FX33 BCD: 0 -> [0, 0, 0]\n");
}

void test_opcode_fx33_bcd_255() {
    Chip8 c;
    c.reset();
    c.index_register = 0x300;
    c.registers[0] = 255;
    place_opcode(c, 0x200, 0xF033);
    c.emulateCycle();
    TEST_ASSERT(c.memory[0x300] == 2);
    TEST_ASSERT(c.memory[0x301] == 5);
    TEST_ASSERT(c.memory[0x302] == 5);
    printf("PASS  FX33 BCD: 255 -> [2, 5, 5]\n");
}

// ── FX55 ────────────────────────────────────────────────────────

void test_opcode_fx55_ld_i_vx() {
    Chip8 c;
    c.reset();
    for (int i = 0; i < 8; i++)
        c.registers[i] = 0x10 + i;
    c.index_register = 0x300;
    place_opcode(c, 0x200, 0xF855);
    c.emulateCycle();
    for (int i = 0; i <= 8; i++)
        TEST_ASSERT(c.memory[0x300 + i] == 0x10 + i);
    printf("PASS  FX55 LD [I], V8 stores V0..V8 at memory[0x300..0x308]\n");
}

void test_opcode_fx55_ld_i_v0() {
    Chip8 c;
    c.reset();
    c.registers[0] = 0xAB;
    c.registers[1] = 0xCD;
    c.index_register = 0x300;
    place_opcode(c, 0x200, 0xF055);
    c.emulateCycle();
    TEST_ASSERT(c.memory[0x300] == 0xAB);
    TEST_ASSERT(c.memory[0x301] == 0x00);
    printf("PASS  FX55 LD [I], V0 stores only V0 at memory[0x300]\n");
}

void test_opcode_fx55_ld_i_vf() {
    Chip8 c;
    c.reset();
    for (int i = 0; i < 16; i++)
        c.registers[i] = i;
    c.index_register = 0x300;
    place_opcode(c, 0x200, 0xFF55);
    c.emulateCycle();
    for (int i = 0; i <= 0xF; i++)
        TEST_ASSERT(c.memory[0x300 + i] == i);
    printf("PASS  FX55 LD [I], VF stores all 16 registers\n");
}

// ── FX65 ────────────────────────────────────────────────────────

void test_opcode_fx65_ld_vx_i() {
    Chip8 c;
    c.reset();
    for (int i = 0; i <= 8; i++)
        c.memory[0x300 + i] = 0x20 + i;
    c.index_register = 0x300;
    place_opcode(c, 0x200, 0xF865);
    c.emulateCycle();
    for (int i = 0; i <= 8; i++)
        TEST_ASSERT(c.registers[i] == 0x20 + i);
    printf("PASS  FX65 LD V8, [I] loads V0..V8 from memory[0x300..0x308]\n");
}

void test_opcode_fx65_ld_vf_i() {
    Chip8 c;
    c.reset();
    for (int i = 0; i < 16; i++)
        c.memory[0x300 + i] = 0xA0 + i;
    c.index_register = 0x300;
    place_opcode(c, 0x200, 0xFF65);
    c.emulateCycle();
    for (int i = 0; i < 16; i++)
        TEST_ASSERT(c.registers[i] == 0xA0 + i);
    printf("PASS  FX65 LD VF, [I] loads all 16 registers\n");
}

// ── IBM Logo Integration ────────────────────────────────────────

void test_ibm_logo_rom() {
    Chip8 c;
    c.reset();
    bool loaded = c.loadROM("roms/IBMLogo.ch8");
    TEST_ASSERT(loaded == true);
    for (int i = 0; i < 22; i++)
        c.emulateCycle();
    int pixels_on = 0;
    for (int i = 0; i < 64 * 32; i++)
        if (c.display[i]) pixels_on++;
    TEST_ASSERT(pixels_on > 0);
    TEST_ASSERT(c.display[8 * 64 + 12] == 1);
    printf("PASS  IBM logo ROM draws %d pixels\n", pixels_on);
}

// ── Main ────────────────────────────────────────────────────────

static int run_tests() {
    test_failures = 0;

    // Fetch / flow
    RUN_TEST(test_fetch_increments_pc);
    RUN_TEST(test_sequential_opcodes);

    // 0NNN  (implemented: 00E0)
    RUN_TEST(test_opcode_00e0_clears_display);

    // 1NNN (implemented: 1NNN)
    RUN_TEST(test_opcode_1nnn_jump);
    RUN_TEST(test_opcode_1nnn_jump_to_zero);

    // 6XNN (implemented: 6XNN)
    RUN_TEST(test_opcode_6xnn_set_register);
    RUN_TEST(test_opcode_6xnn_set_register_zero);

    // 7XNN (implemented: 7XNN)
    RUN_TEST(test_opcode_7xnn_add);
    RUN_TEST(test_opcode_7xnn_add_wraps);

    // ANNN (implemented: ANNN)
    RUN_TEST(test_opcode_annn_set_index);
    RUN_TEST(test_opcode_annn_set_index_high);

    // DXYN (implemented: DXYN)
    RUN_TEST(test_opcode_dxyn_draw_turns_pixels_on);
    RUN_TEST(test_opcode_dxyn_xor_collision);
    RUN_TEST(test_opcode_dxyn_no_collision_clears_vf);
    RUN_TEST(test_opcode_dxyn_wraps_x);
    RUN_TEST(test_opcode_dxyn_wraps_y);
    RUN_TEST(test_opcode_dxyn_multi_row);
    RUN_TEST(test_opcode_dxyn_vf_not_affected_when_sprite_byte_zero);

    // Integration
    RUN_TEST(test_ibm_logo_rom);

    printf("\n=== Implemented opcode tests done ===\n\n");

    // ── NOT YET IMPLEMENTED ────────────────────────────────────
    // The tests below will fail until their opcodes are implemented.

    // 00EE
    RUN_TEST(test_opcode_00ee_return);
    RUN_TEST(test_opcode_00ee_return_underflow);

    // 2NNN
    RUN_TEST(test_opcode_2nnn_call);
    RUN_TEST(test_opcode_2nnn_call_nested);
    RUN_TEST(test_opcode_2nnn_call_stack_overflow);

    // 3XNN
    RUN_TEST(test_opcode_3xnn_skip_equal);
    RUN_TEST(test_opcode_3xnn_no_skip_not_equal);

    // 4XNN
    RUN_TEST(test_opcode_4xnn_skip_not_equal);
    RUN_TEST(test_opcode_4xnn_no_skip_equal);

    // 5XY0
    RUN_TEST(test_opcode_5xy0_skip_equal);
    RUN_TEST(test_opcode_5xy0_no_skip_not_equal);

    // 8XY0-8XYE
    RUN_TEST(test_opcode_8xy0_ld);
    RUN_TEST(test_opcode_8xy1_or);
    RUN_TEST(test_opcode_8xy2_and);
    RUN_TEST(test_opcode_8xy3_xor);
    RUN_TEST(test_opcode_8xy4_add_no_carry);
    RUN_TEST(test_opcode_8xy4_add_with_carry);
    RUN_TEST(test_opcode_8xy4_add_carry_exact_256);
    RUN_TEST(test_opcode_8xy5_sub_no_borrow);
    RUN_TEST(test_opcode_8xy5_sub_with_borrow);
    RUN_TEST(test_opcode_8xy5_sub_equal);
    RUN_TEST(test_opcode_8xy6_shr_lsb_zero);
    RUN_TEST(test_opcode_8xy6_shr_lsb_one);
    RUN_TEST(test_opcode_8xy7_subn_no_borrow);
    RUN_TEST(test_opcode_8xy7_subn_with_borrow);
    RUN_TEST(test_opcode_8xye_shl_msb_zero);
    RUN_TEST(test_opcode_8xye_shl_msb_one);

    // 9XY0
    RUN_TEST(test_opcode_9xy0_skip_not_equal);
    RUN_TEST(test_opcode_9xy0_no_skip_equal);

    // BNNN
    RUN_TEST(test_opcode_bnnn_jump_offset);
    RUN_TEST(test_opcode_bnnn_jump_offset_zero);

    // CXNN
    RUN_TEST(test_opcode_cxnn_random_masked);
    RUN_TEST(test_opcode_cxnn_random_mask_zero);

    // EX9E / EXA1
    RUN_TEST(test_opcode_ex9e_skip_key_pressed);
    RUN_TEST(test_opcode_ex9e_no_skip_key_not_pressed);
    RUN_TEST(test_opcode_exa1_skip_key_not_pressed);
    RUN_TEST(test_opcode_exa1_no_skip_key_pressed);

    // FX07
    RUN_TEST(test_opcode_fx07_ld_vx_dt);

    // FX0A
    RUN_TEST(test_opcode_fx0a_wait_key_blocks);
    RUN_TEST(test_opcode_fx0a_wait_key_stores_key);
    RUN_TEST(test_opcode_fx0a_wait_key_picks_first_pressed);

    // FX15
    RUN_TEST(test_opcode_fx15_ld_dt_vx);

    // FX18
    RUN_TEST(test_opcode_fx18_ld_st_vx);

    // FX1E
    RUN_TEST(test_opcode_fx1e_add_i_vx);
    RUN_TEST(test_opcode_fx1e_add_i_vx_wrap);

    // FX29
    RUN_TEST(test_opcode_fx29_ld_font_sprite);
    RUN_TEST(test_opcode_fx29_ld_font_sprite_digit_f);
    RUN_TEST(test_opcode_fx29_ld_font_sprite_vx_beyond_f);

    // FX33
    RUN_TEST(test_opcode_fx33_bcd);
    RUN_TEST(test_opcode_fx33_bcd_zero);
    RUN_TEST(test_opcode_fx33_bcd_255);

    // FX55
    RUN_TEST(test_opcode_fx55_ld_i_vx);
    RUN_TEST(test_opcode_fx55_ld_i_v0);
    RUN_TEST(test_opcode_fx55_ld_i_vf);

    // FX65
    RUN_TEST(test_opcode_fx65_ld_vx_i);
    RUN_TEST(test_opcode_fx65_ld_vf_i);

    printf("\n=== ALL TESTS COMPLETE: %d failures ===\n", test_failures);
    return test_failures > 0 ? 1 : 0;
}

int main() {
    return run_tests();
}
