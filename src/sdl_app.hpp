#pragma once
#include <SDL.h>
#include <cstdint>
#include <functional>
#include "tinymsx_def.h"
#include "sdl_emu.hpp"

class SDL_App {

public:
    SDL_Emu* emu;
    inline SDL_App(SDL_Renderer* r, std::vector<uint8_t> rom, std::vector<uint8_t> bios)
    {
        emu = new SDL_Emu();
        tex = SDL_CreateTexture(
            r,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            VRAM_WIDTH, VRAM_HEIGHT
        );

        framebuffer = new uint32_t[VRAM_WIDTH * VRAM_HEIGHT];

        std::memset(framebuffer, 0, VRAM_WIDTH * VRAM_HEIGHT * sizeof(uint32_t));
        emu->init(rom, bios);
    }
    inline void destroy() {
        if (tex) {
            SDL_DestroyTexture(tex);
        }
        emu->destroy();
    }
    inline ~SDL_App() {
        delete[] framebuffer;
        delete emu;
    }

    // ---- VSync 相当（毎フレーム呼ぶ）----
    inline void vsync(SDL_Renderer* renderer) {
        SDL_RenderPresent(renderer);
        emu->vsync();
        drawVRAM(reinterpret_cast<uint16_t*>(emu->emu_vram));
        SDL_RenderCopy(renderer, tex, nullptr, nullptr);
    }

    // emu_vram (RGB555) を渡して描画更新
    inline void drawVRAM(uint16_t* emu_vram)
    {
        uint32_t* dst = framebuffer;

        const int n = VRAM_WIDTH * VRAM_HEIGHT;
        for (int i = 0; i < n; i++) {
            dst[i] = rgb555_to_argb8888(emu_vram[i]);
        }

        SDL_UpdateTexture(tex, nullptr, dst, VRAM_WIDTH * sizeof(uint32_t));
    }

    // ---- SDL Event ----
    inline void handleEvent(const SDL_Event& ev)
    {
        switch (ev.type) {
        case SDL_KEYDOWN:
            onKey(ev.key.keysym.sym, true);
            break;
        case SDL_KEYUP:
            onKey(ev.key.keysym.sym, false);
            break;
        default:
            break;
        }
    }

private:
    SDL_Texture* tex = nullptr;
    uint32_t* framebuffer;

    inline uint32_t rgb555_to_argb8888(uint16_t p) const
    {
        uint8_t r = (p >> 10) & 0x1F;
        uint8_t g = (p >> 5) & 0x1F;
        uint8_t b = (p >> 0) & 0x1F;

        // 5bit → 8bit
        r = (r << 3) | (r >> 2);
        g = (g << 3) | (g >> 2);
        b = (b << 3) | (b >> 2);

        return 0xFF000000
            | (uint32_t(r) << 16)
            | (uint32_t(g) << 8)
            | uint32_t(b);
    }

    inline void onKey(SDL_Keycode key, bool down)
    {
        uint8_t mask = 0;

        switch (key) {
        case SDLK_RIGHT: mask = TINYMSX_JOY_RI; break;
        case SDLK_LEFT:  mask = TINYMSX_JOY_LE; break;
        case SDLK_DOWN:  mask = TINYMSX_JOY_DW; break;
        case SDLK_UP:    mask = TINYMSX_JOY_UP; break;

        case SDLK_RETURN: mask = TINYMSX_JOY_T2; break;
        case SDLK_SPACE:  mask = TINYMSX_JOY_T1; break;

        case SDLK_x: mask = TINYMSX_JOY_T2; break;
        case SDLK_z: mask = TINYMSX_JOY_T1; break;

        case SDLK_1: mask = TINYMSX_JOY_S1; break;
        case SDLK_2: mask = TINYMSX_JOY_S2; break;

        case SDLK_d:
            if (down) emu->printDump();
            return;
        case SDLK_s:
            if (down) emu->keepRam();
            return;
        case SDLK_e:
            if (down) emu->compareRam();
            return;
        case SDLK_h:
            if (down) emu->reset();
            return;
        default:
            return;
        }

        if (down)
            emu->key |= mask;
        else
            emu->key &= ~mask;
    }
};
