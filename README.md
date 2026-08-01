# CHIP-8 Emulator

A CHIP-8 emulator written in C++ with SDL2, built to learn low-level emulation concepts.

## Features

- All 35 standard CHIP-8 opcodes implemented and tested
- SDL2 rendering at 60 FPS (640x320 window, 10x scale from 64x32)
- 60Hz timer system (delay timer, sound timer)
- Audio beep via SDL audio (square wave at ~550Hz when sound timer is active)
- Keyboard input mapped to CHIP-8 hex keypad
- IBM logo and 24 game ROMs included
- Test suite: 68 opcode tests, CPU reset tests, ROM loading tests

## Prerequisites

- **Compiler**: MinGW-w64 (g++) with C++17 support
- **Build tool**: GNU Make (or `mingw32-make` on Windows)
- **SDL2**: Prebuilt SDL2-2.30.x development libraries (included in `lib/`)

### SDL2 Setup (if not included)

The `lib/` directory contains prebuilt SDL2 MinGW libraries and `SDL2.dll`.
If you need to download them yourself:

1. Download `SDL2-devel-2.30.x-mingw.zip` from [SDL2 Releases](https://github.com/libsdl-org/SDL/releases)
2. Extract `i686-w64-mingw32/` (32-bit) or `x86_64-w64-mingw32/` (64-bit) contents to `lib/`
3. Copy `SDL2.dll` from `lib/bin/` to `lib/`

## Building

```bash
make
```

Or manually:

```bash
g++ -Iinclude -Wall src/main.cpp src/chip8.cpp -Llib -lmingw32 -lSDL2main -lSDL2 -o build/chip8
cp lib/SDL2.dll build/
```

## Running

```bash
make run ROM=PONG
```

Or manually:

```bash
./build/chip8.exe roms/PONG
```

### Available ROMs

| ROM        | Description                    |
|------------|--------------------------------|
| `IBMLogo.ch8` | IBM logo test pattern       |
| `PONG`     | Classic Pong                   |
| `PONG2`    | Pong variant                   |
| `BRIX`     | Breakout clone                 |
| `VBRIX`    | Vertical breakout              |
| `INVADERS` | Space Invaders clone           |
| `TETRIS`   | Tetris                         |
| `TANK`     | Tank battle game               |
| `BLINKY`   | Maze game                      |
| `BLITZ`    | Blitz game                     |
| `CONNECT4` | Connect Four                   |
| `GUESS`    | Number guessing game           |
| `HIDDEN`   | Hidden picture game            |
| `KALEID`   | Kaleidoscope pattern           |
| `MAZE`     | Maze generator                 |
| `MERLIN`   | Merlin game                    |
| `MISSILE`  | Missile command                |
| `PUZZLE`   | Puzzle game                    |
| `SYZYGY`   | Syzygy game                    |
| `UFO`      | UFO game                       |
| `VERS`     | Versus game                    |
| `WIPEOFF`  | Wipe-off game                  |
| `15PUZZLE` | 15-puzzle                      |
| `TICTAC`   | Tic-Tac-Toe                    |

## Controls

CHIP-8 uses a 16-key hex keypad mapped to your keyboard:

```
CHIP-8 Keypad       Keyboard
----------------   ---------
1 2 3 C            1 2 3 4
4 5 6 D            Q W E R
7 8 9 E            A S D F
A 0 B F            Z X C V
```

- **Escape**: Quit the emulator

## Makefile Targets

| Command                     | Action                              |
|-----------------------------|-------------------------------------|
| `make`                      | Build the emulator                  |
| `make build`                | Build the emulator                  |
| `make run`                  | Build and run (default: IBMLogo.ch8)|
| `make run ROM=PONG`         | Build and run a specific ROM        |
| `make tests`                | Build and run all tests             |
| `make tests/test_opcodes`   | Build and run opcode tests          |
| `make clean`                | Remove build directory              |

## Project Structure

```
chip8-emulator/
├── include/
│   └── chip8.h          # Chip8 CPU struct and declarations
├── src/
│   ├── chip8.cpp        # CPU core: opcodes, timers, fontset, ROM loading
│   └── main.cpp         # Entry point, SDL render loop, audio, input
├── tests/
│   ├── test_cpu.cpp     # CPU reset state tests (6 tests)
│   ├── test_opcodes.cpp # All 35 opcode tests (68 tests)
│   └── test_rom.cpp     # ROM loading tests (4 tests)
├── lib/                  # Prebuilt SDL2 libraries and DLL
├── roms/                 # CHIP-8 game ROMs
├── Makefile
└── README.md
```

## How It Works

### Emulation Cycle

1. **Fetch**: Read 2-byte opcode from `memory[program_counter]`
2. **Decode**: Extract nibbles (category, X, Y, N, NN, NNN)
3. **Execute**: Run the corresponding opcode handler (switch on `category`)
4. **Repeat**: ~540 cycles per frame (~9 cycles per render frame at 60 FPS)

### Memory Map

| Address Range | Content                  |
|---------------|--------------------------|
| `0x000-0x04F`| Reserved / interpreter   |
| `0x050-0x09F`| Fontset (80 bytes)       |
| `0x0A0-0x1FF`| Available for programs   |
| `0x200-0xFFF`| ROM program code/data    |

### Timers

- **delay_timer**: Decrements at 60Hz. Used for game timing.
- **sound_timer**: Decrements at 60Hz. Beeps while > 0.

Both timers are updated once per frame (every ~16ms) based on real elapsed time via `SDL_GetTicks()`.

### Display

- Resolution: 64x32 pixels
- Window: 640x320 (10x scale)
- White pixels on black background
- Updated at 60 FPS

## Tests

Run all tests:

```bash
make tests
```

Run a specific test:

```bash
make tests/test_opcodes
make tests/test_cpu
make tests/test_rom
```

## Implemented Opcodes

| Opcode   | Mnemonic        | Description                         |
|----------|-----------------|-------------------------------------|
| `00E0`   | CLS             | Clear display                       |
| `00EE`   | RET             | Return from subroutine              |
| `1NNN`   | JP addr         | Jump to address NNN                 |
| `2NNN`   | CALL addr       | Call subroutine at NNN              |
| `3XNN`   | SE VX, byte     | Skip if VX == NN                    |
| `4XNN`   | SNE VX, byte    | Skip if VX != NN                    |
| `5XY0`   | SE VX, VY       | Skip if VX == VY                    |
| `6XNN`   | LD VX, byte     | Set VX = NN                         |
| `7XNN`   | ADD VX, byte    | VX += NN                            |
| `8XY0`   | LD VX, VY       | Set VX = VY                         |
| `8XY1`   | OR VX, VY       | VX \|= VY                           |
| `8XY2`   | AND VX, VY      | VX &= VY                            |
| `8XY3`   | XOR VX, VY      | VX ^= VY                            |
| `8XY4`   | ADD VX, VY      | VX += VY, VF = carry                |
| `8XY5`   | SUB VX, VY      | VX -= VY, VF = !borrow              |
| `8XY6`   | SHR VX          | VX >>= 1, VF = LSB                  |
| `8XY7`   | SUBN VX, VY     | VX = VY - VX, VF = !borrow          |
| `8XYE`   | SHL VX          | VX <<= 1, VF = MSB                  |
| `9XY0`   | SNE VX, VY      | Skip if VX != VY                    |
| `ANNN`   | LD I, addr      | Set index_register = NNN            |
| `BNNN`   | JP V0, addr     | Jump to NNN + V0                    |
| `CXNN`   | RND VX, byte    | VX = random & NN                    |
| `DXYN`   | DRW VX, VY, nib | Draw sprite at (VX, VY), N rows     |
| `EX9E`   | SKP VX          | Skip if key VX pressed              |
| `EXA1`   | SKNP VX         | Skip if key VX not pressed          |
| `FX07`   | LD VX, DT       | VX = delay_timer                    |
| `FX0A`   | LD VX, K        | Wait for keypress, store in VX      |
| `FX15`   | LD DT, VX       | delay_timer = VX                    |
| `FX18`   | LD ST, VX       | sound_timer = VX                    |
| `FX1E`   | ADD I, VX       | index_register += VX                |
| `FX29`   | LD F, VX        | Set I to font sprite for hex digit VX|
| `FX33`   | LD B, VX        | Store BCD of VX at I, I+1, I+2      |
| `FX55`   | LD [I], VX      | Store V0..VX at memory[I..]         |
| `FX65`   | LD VX, [I]      | Load V0..VX from memory[I..]        |

## Youtube Video coming soon
