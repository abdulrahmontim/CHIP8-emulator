#include <SDL2/SDL.h>
#include <iostream>
#include <chip8.h>

using namespace std;


const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 320;
const int SCALE = 10;

static bool beep_active = false;

void audio_callback(void* userdata, Uint8* stream, int len) {
    for (int i = 0; i < len; i++) {
        stream[i] = (beep_active && (i % 80 < 40)) ? 200 : 128;
    }
}

uint8_t mapKey(int sym) {
    switch (sym) {
        case '1': return 0x1; case '2': return 0x2; case '3': return 0x3; case '4': return 0xC;
        case 'q': return 0x4; case 'w': return 0x5; case 'e': return 0x6; case 'r': return 0xD;
        case 'a': return 0x7; case 's': return 0x8; case 'd': return 0x9; case 'f': return 0xE;
        case 'z': return 0xA; case 'x': return 0x0; case 'c': return 0xB; case 'v': return 0xF;
        default: return 0xFF;
    }
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <ROM_File>" << endl;
        return 1;
    }

    Chip8 chip = Chip8();
    chip.reset();

    if (!chip.loadROM(argv[1])) {
        cerr << "Failed to load ROM!" << endl;
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "CHIP8-Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (window == nullptr) {
        cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << endl;
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (renderer == nullptr) {
        cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << endl;
        return 1;
    }

    bool running = true;
    SDL_Event event;
    uint32_t lastTimerUpdate = SDL_GetTicks();

    while (running) {
        uint32_t now = SDL_GetTicks();
        if (now - lastTimerUpdate >= 16) {
            if (chip.delay_timer > 0) chip.delay_timer--;
            if (chip.sound_timer > 0) chip.sound_timer--;
            lastTimerUpdate = now;
        }

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) running = false;
                uint8_t key = mapKey(event.key.keysym.sym);
                if (key != 0xFF) chip.keypad[key] = true;
            }

            if (event.type == SDL_KEYUP) {
                uint8_t key = mapKey(event.key.keysym.sym);
                if (key != 0xFF) chip.keypad[key] = false;
            }
        }

        for (int i = 0; i < 9; i++) chip.emulateCycle();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 64; x++) {
                if (chip.display[y * 64 + x]) {
                    SDL_Rect rect = { x * SCALE, y * SCALE, SCALE, SCALE };
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
