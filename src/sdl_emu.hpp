//
//  sdl_emu.hpp
//
//  Created by 鈴木　洋司　 on 2018/12/30.
//  Copyright © 2018年 SUZUKIPLAN. All rights reserved.
//
#pragma once
#include <stdio.h>
#define VRAM_WIDTH 284          // 映像領域の大きさ
#define VRAM_HEIGHT 240         // 映像領域の高さ
#include <vector>
#include "sdl_audio.hpp"
#include "tinymsx.hpp"
#include "tinymsx_def.h"

struct SDL_Emu {
    unsigned short emu_vram[VRAM_WIDTH * VRAM_HEIGHT];
    unsigned char key;
    inline static volatile unsigned short sound_cursor;
    inline static pthread_mutex_t sound_locker;
    inline static char sound_buffer[65536 * 4];
    SDL_Audio* spu;
    TinyMSX* emu_msx = NULL;
    inline static void sound_callback(void* buffer, size_t size)
    {
        if (sound_cursor < size) {
            return;
        }
        pthread_mutex_lock(&sound_locker);
        memcpy(buffer, sound_buffer, size);
        if (size <= sound_cursor) {
            memcpy(sound_buffer, sound_buffer + size, sound_cursor - size);
            sound_cursor -= size;
        }
        else {
            printf("size = %d cursor %d\n", size, sound_cursor);
            sound_cursor = 0;
        }
        pthread_mutex_unlock(&sound_locker);
    }

    inline static int getTypeOfRom(char* rom, size_t romSize)
    {
        if (0x8000 < romSize) {
            puts("open as MSX1 ASC8 mega-rom file");
            return TINYMSX_TYPE_MSX1_ASC8;
        }
        else if (64 * 1024 == romSize) {
            puts("open as MSX1 64KB ROM file");
            return TINYMSX_TYPE_MSX1;
        }
        else {
            puts("open as MSX1 rom file");
            return TINYMSX_TYPE_MSX1;
        }
    }
    std::vector<uint8_t> bios;
    /**
     * 起動時に1回だけ呼び出される
     */
    inline void init(std::vector<uint8_t> rom, std::vector<uint8_t> bios)
    {
        printf("load rom (size: %lu)\n", rom.size());
        this->bios = bios;
        reload(rom);
        pthread_mutex_init(&sound_locker, NULL);
        spu = new SDL_Audio();
        spu->init(44100, 16, 2, 512, sound_callback);
    }

    inline void reload(std::vector<uint8_t> rom)
    {
        puts("emu_reload");
        if (emu_msx) {
            delete emu_msx;
            emu_msx = NULL;
        }
        emu_msx = new TinyMSX(getTypeOfRom((char*)rom.data(), rom.size()), rom.data(), rom.size(), 0x4000, TINYMSX_COLOR_MODE_RGB555);
        emu_msx->loadBiosFromMemory(bios.data(), bios.size());
        emu_msx->reset();
        emu_msx->setupSpecialKey1('1', 0);
        emu_msx->setupSpecialKey2(' ', 0);
    }

    inline void reset()
    {
        emu_msx->reset();
    }

    /**
     * 画面の更新間隔（1秒間で60回）毎にこの関数がコールバックされる
     * この中で以下の処理を実行する想定:
     * 1. エミュレータのCPU処理を1フレーム分実行
     * 2. エミュレータの画面バッファの内容をvramへコピー
     */
    inline void vsync()
    {
        if (!emu_msx) return;
        emu_msx->tick(key, 0);
        const void* ptr = emu_msx->getDisplayBuffer();
        memcpy(emu_vram, ptr, sizeof(emu_vram));
        size_t pcmSize;
        void* pcm = emu_msx->getSoundBuffer(&pcmSize);
        //printf("pcmSize = %d\n", pcmSize);
        pthread_mutex_lock(&sound_locker);
        if (pcmSize + sound_cursor < sizeof(sound_buffer)) {
            memcpy(&sound_buffer[sound_cursor], pcm, pcmSize);
            sound_cursor += pcmSize;
        }
        else {
            printf("buffer error pcmSize = %d cursor = %d\n", pcmSize, sound_cursor);
        }
        pthread_mutex_unlock(&sound_locker);
    }

    /**
     * 終了時に1回だけ呼び出される
     * この中でエミュレータの初期化処理を実行する想定
     */
    inline void destroy()
    {
        puts("emu_destroy");
        spu->destroy();
        delete spu;

        if (emu_msx) {
            delete emu_msx;
            emu_msx = NULL;
        }
        pthread_mutex_destroy(&sound_locker);
    }

    inline const void* saveState(size_t* size)
    {
        if (!emu_msx) return NULL;
        return emu_msx->saveState(size);
    }

    inline void loadState(const void* state, size_t size)
    {
        if (!emu_msx) return;
        emu_msx->loadState(state, size);
    }

    inline void printDump()
    {
        if (!emu_msx) return;
        unsigned char* vram = emu_msx->tms9918->ctx.ram;
        unsigned short nameTableAddr = emu_msx->tms9918->ctx.reg[2] << 10;
        puts("[NAME TABLE]");
        printf("      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F\n");
        for (int i = 0; i < 24; i++) {
            printf("%04X:", nameTableAddr);
            for (int j = 0; j < 32; j++) {
                printf(" %02X", vram[nameTableAddr++]);
            }
            printf("\n");
        }
    }
    unsigned char ramKeep[0x4000];

    inline void keepRam()
    {
        if (!emu_msx) return;
        memcpy(ramKeep, emu_msx->ram, 0x4000);
    }

    inline void compareRam()
    {
        unsigned char ramCompare[0x4000];
        if (!emu_msx) return;
        memcpy(ramCompare, emu_msx->ram, 0x4000);
        puts("[RAM COMPARE]");
        for (int i = 0; i < 0x4000; i++) {
            if (ramKeep[i] != ramCompare[i]) {
                printf("- %04X: %02X -> %02X\n", i, ramKeep[i], ramCompare[i]);
            }
        }
    }

};
