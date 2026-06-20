#include <SDL2/SDL.h>
#include <iostream>
#include <chip8.h>

using namespace std;


const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 320;


int main(int argc, char* argv[]) {
    Chip8 chip = Chip8();
    chip.reset();
    
    if (!chip.loadROM("roms/IBMLogo.ch8")) {
        cerr << "Failed to load ROM!" << endl;
        return 1;
    }
    
    cout << "ROM loaded. Running 500 cycles..." << endl;
    for (int i = 0; i < 500; i++) {
        chip.emulateCycle();
    }

    chip.debugPrintDisplay();
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << endl;
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
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << endl;
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << endl;
        return 1;
    }

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
    }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

