/* (C)2016, SUZUKI PLAN.
 *----------------------------------------------------------------------------
 * Description: VGS - Sound Processing Unit
 *    Platform: Common
 *      Author: Yoji Suzuki (SUZUKI PLAN)
 *----------------------------------------------------------------------------
 */
#ifndef INCLUDE_VGSSPU_H
#define INCLUDE_VGSSPU_H
#include <stdio.h>
#include <SDL.h>

struct SDL_Audio {
    SDL_AudioDeviceID dev;
    SDL_AudioSpec spec;

    void* buffer;
    size_t size;

    void (*callback)(void* buffer, size_t size);

    int alive;

    inline static void sdl_audio_callback(void* userdata, Uint8* stream, int len)
    {
        SDL_Audio* c = (SDL_Audio*)userdata;
        if (!c->alive) {
            memset(stream, 0, len);
            return;
        }

        if ((size_t)len != c->size) {
            /* size mismatch: silence */
            memset(stream, 0, len);
            return;
        }

        /* emulation side fills buffer */
        c->callback(c->buffer, c->size);

        memcpy(stream, c->buffer, c->size);
    }

    SDL_Audio* init(int sampling, int bit, int ch, size_t size,
        void (*callback)(void* buffer, size_t size))
    {
        SDL_AudioSpec want;

        if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
            if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
                fprintf(stderr, "SDL audio init failed: %s\n", SDL_GetError());
                return NULL;
            }
        }

        this->buffer = malloc(size);
        if (!this->buffer) {
            delete this;
            return NULL;
        }
        memset(this->buffer, 0, size);

        this->callback = callback;
        this->size = size;
        this->alive = 1;

        SDL_zero(want);
        want.freq = sampling;
        want.channels = ch;
        want.samples = (Uint16)(size / (bit / 8) / ch);
        want.callback = this->sdl_audio_callback;
        want.userdata = this;

        if (bit == 16) {
            want.format = AUDIO_S16SYS;
        }
        else if (bit == 8) {
            want.format = AUDIO_U8;
        }
        else {
            free(this->buffer);
            delete this;
            return NULL;
        }

        this->dev = SDL_OpenAudioDevice(NULL, 0, &want, &this->spec, 0);
        if (this->dev == 0) {
            fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
            free(this->buffer);
            delete this;
            return NULL;
        }
        printf("open audio device %d\n", this->dev);
        SDL_PauseAudioDevice(this->dev, 0); /* start audio */
        return this;
    }

    void destroy()
    {
        alive = 0;

        if (dev) {
            SDL_CloseAudioDevice(dev);
        }

        if (buffer) {
            free(buffer);
        }

        printf("free context\n");
    }

};

#endif
