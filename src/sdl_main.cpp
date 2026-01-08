

#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cstring>
#include "sdl_app.hpp"

static std::vector<uint8_t> loadFile(const char* path)
{
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        perror(path);
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::vector<uint8_t> buf(size);
    fread(buf.data(), 1, size, fp);
    fclose(fp);
    return buf;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("usage: %s romfile\n", argv[0]);
        return 1;
    }

    auto rom = loadFile(argv[1]);
    auto bios = loadFile("../bios/cbios_main_msx1.rom");

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    const int SCALE = 3;

    SDL_Window* win = SDL_CreateWindow(
        "tinymsx",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        VRAM_WIDTH * SCALE,
        VRAM_HEIGHT * SCALE,
        SDL_WINDOW_RESIZABLE
    );

    SDL_Renderer* ren = SDL_CreateRenderer(
        win,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    SDL_RenderSetLogicalSize(ren, VRAM_WIDTH, VRAM_HEIGHT);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    SDL_App app(ren, rom, bios);

    bool running = true;

    while (running) {
        app.vsync(ren);
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)
                running = false;
            app.handleEvent(ev);
        }
    }
    app.destroy();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
