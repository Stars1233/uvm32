#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <getopt.h>
#include "uvm32.h"

#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>

#include "../common/uvm32_common_custom.h"

#define WIDTH 320
#define HEIGHT 200

#define WINDOW_WIDTH WIDTH*3
#define WINDOW_HEIGHT HEIGHT*3

// circular buffer of keypresses, so vm code can read them as it's ready
typedef struct {
    bool down;
    uint16_t scancode;
} keyevent_t;

#define KEYBUFFER_LEN 8
static keyevent_t keyBuffer[KEYBUFFER_LEN];
static int keyBufferWr = 0;
static int keyBufferRd = 0;

// circular buffer of audio samples, so vm code can read them as it's ready
#define AUDIOBUFFER_LEN 4096
static int16_t audioBuffer[AUDIOBUFFER_LEN];
static int audioBufferWr = 0;
static int audioBufferRd = 0;

void key_enq(uint16_t scancode, bool down) {
    keyBuffer[keyBufferWr].scancode = scancode;
    keyBuffer[keyBufferWr].down = down;

    keyBufferWr = (keyBufferWr + 1) % KEYBUFFER_LEN;
}

bool key_deq(keyevent_t *ke) {
   if (keyBufferWr != keyBufferRd) {
        *ke = keyBuffer[keyBufferRd];
        keyBufferRd = (keyBufferRd + 1) % KEYBUFFER_LEN;
        return true;
    } else {
        return false;
    }
}

void audio_enq(int16_t sample) {
    audioBuffer[audioBufferWr] = sample;
    audioBufferWr = (audioBufferWr + 1) % AUDIOBUFFER_LEN;
}

bool audio_deq(int16_t *sample) {
   if (audioBufferWr != audioBufferRd) {
        *sample = audioBuffer[audioBufferRd];
        audioBufferRd = (audioBufferRd + 1) % AUDIOBUFFER_LEN;
        return true;
    } else {
        return false;
    }
}


static uint8_t *read_file(const char* filename, int *len) {
    FILE* f = fopen(filename, "rb");
    uint8_t *buf = NULL;

    if (f == NULL) {
        fprintf(stderr, "error: can't open file '%s'.\n", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    rewind(f);

    if (NULL == (buf = malloc(file_size))) {
        fclose(f);
        return NULL;
    }

    size_t result = fread(buf, sizeof(uint8_t), file_size, f);
    if (result != file_size) {
        fprintf(stderr, "error: while reading file '%s'\n", filename);
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);

    *len = file_size;
    return buf;
}

bool memdump(const char *filename, const uint8_t *buf, int len) {
    FILE* f = fopen(filename, "wb");

    if (f == NULL) {
        fprintf(stderr, "error: can't open file '%s'.\n", filename);
        return false;
    }

    size_t result = fwrite(buf, sizeof(uint8_t), len, f);
    if (result != len) {
        fprintf(stderr, "error: while writing file '%s'\n", filename);
        return false;
    }
    fclose(f);
    return true;
}

void usage(const char *name) {
    printf("%s [options] filename.bin\n", name);
    printf("Options:\n");
    printf("  -h                            show help\n");
    printf("  -i <num instructions>         max instrs before requiring a syscall\n");
    printf("  -e <extram size>              numbers of bytes for extram\n");
    exit(1);
}

void audiocb(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
    if (audioBufferWr == audioBufferRd) { // empty
        return;
    }
    
    while(total_amount > 0) {
        int16_t sample[2];

        // always expect audio in pairs
        if (audio_deq(&sample[0])) {
            if (audio_deq(&sample[1])) {
                if (!SDL_PutAudioStreamData(stream, sample, 4)) {
                    printf("audio write failed\r\n");
                }
            }
        } else {
            return;
        }
        total_amount -= 4;
    }
}


int main(int argc, char *argv[]) {
    uvm32_state_t *vmst = NULL;
    uint32_t max_instrs_per_run = 500000;
    char c;
    const char *rom_filename = NULL;
    uint32_t extram_len = 0;
    uint32_t *extram_buf = NULL;
    uvm32_evt_t evt;
    bool isrunning = true;
    uint32_t total_instrs = 0;
    uint32_t num_syscalls = 0;
    int romlen = 0;
    SDL_Renderer *renderer = NULL;
    SDL_Window *screen = NULL;
    SDL_Event event;
    SDL_Texture *render_target = NULL;

    // memory for vmst is very large, so allocate
    vmst = (uvm32_state_t *)malloc(sizeof(uvm32_state_t));
    if (vmst == NULL) {
        printf("Out of memory\n");
        return 1;
    }

    // parse commandline args
    while ((c = getopt(argc, argv, "hi:e:")) != -1) {
        switch(c) {
            case 'h':
                usage(argv[0]);
            break;
            case 'i':
                max_instrs_per_run = strtoll(optarg, NULL, 10);
                if (max_instrs_per_run < 1) {
                    usage(argv[0]);
                }
            break;
            case 'e':
                extram_len = strtoll(optarg, NULL, 10);
            break;
        }
    }
    if (optind < argc) {
        rom_filename = argv[optind];
    } else {
        usage(argv[0]);
    }

    uint8_t *rom = read_file(rom_filename, &romlen);
    if (NULL == rom) {
        printf("file read failed!\n");
        return 1;
    }

    uvm32_init(vmst);

    if (!uvm32_load(vmst, rom, romlen)) {
        printf("load failed!\n");
        return 1;
    }

    if (extram_len > 0) {
        extram_buf = (uint32_t *)malloc(extram_len);
        if (NULL == extram_buf) {
            printf("Failed to allocate extram!\n");
            return 1;
        }
        memset(extram_buf, 0x00, extram_len);
        uvm32_extram(vmst, (uint8_t *)extram_buf, extram_len);
    }

    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        printf("SDL init failed\n");
        return 1;
    }

    screen = SDL_CreateWindow("sdl-host", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    if (NULL == screen) {
        printf("SDL CreateWindow failed\n");
        return 1;
    }
    renderer = SDL_CreateRenderer(screen, NULL);
    if (NULL == renderer) {
        printf("SDL CreateRenderer failed\n");
        return 1;
    }

    render_target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    SDL_SetTextureScaleMode(render_target, SDL_SCALEMODE_NEAREST);

    SDL_AudioSpec srcspec = {
        .format = SDL_AUDIO_S16,
        .channels = 2,
        .freq = 44100,
    };

    SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &srcspec, audiocb, NULL);
    if (NULL == stream) {
        printf("Could not setup audio!\n");
        return 1;
    }
    SDL_ResumeAudioStreamDevice(stream);

    while (isrunning) {
        SDL_PollEvent(&event);

        switch(event.type) {
            case SDL_EVENT_QUIT:
                isrunning = false;
            break;
            case SDL_EVENT_KEY_DOWN:
                if (!event.key.repeat) {
                    key_enq(event.key.scancode, true);
                }
            break;
            case SDL_EVENT_KEY_UP:
                if (!event.key.repeat) {
                    key_enq(event.key.scancode, false);
                }
            break;
        }

        total_instrs += uvm32_run(vmst, &evt, max_instrs_per_run);   // num instructions before vm considered hung
        num_syscalls++;

        switch(evt.typ) {
            case UVM32_EVT_END:
                printf("UVM32_EVT_END\n");
                isrunning = false;
            break;
            case UVM32_EVT_ERR:
                printf("UVM32_EVT_ERR '%s' (%d)\n", evt.data.err.errstr, (int)evt.data.err.errcode);
                if (evt.data.err.errcode == UVM32_ERR_HUNG) {
                    printf("VM may have hung, increase max_instrs_per_run\n");
                    uvm32_clearError(vmst);    // allow to continue
                } else {
                    isrunning = false;
                    memdump("host-ram.dump", uvm32_getMemory(vmst), UVM32_MEMORY_SIZE);
                    printf("memory dumped to host-ram.dump, pc=0x%08x\n", uvm32_getProgramCounter(vmst));
                    if (extram_buf != NULL) {
                        memdump("host-extram.dump", (uint8_t *)extram_buf, extram_len);
                        printf("extram dumped to host-extram.dump\n");
                    }
                }
            break;
            case UVM32_EVT_SYSCALL:
                switch(evt.data.syscall.code) {
                    case UVM32_SYSCALL_PRINTBUF: {
                        uvm32_slice_t buf = uvm32_arg_getslice(vmst, &evt, ARG0, ARG1);
                        while(buf.len--) {
                            printf("%02x", *buf.ptr++);
                        }
                    } break;
                    case UVM32_SYSCALL_YIELD: {
                        // uint32_t yield_typ = uvm32_arg_getval(vmst, &evt, ARG0);
                        // printf("YIELD type=%d\n", yield_typ);
                        // uvm32_arg_setval(vmst, &evt, RET, 123);
                    } break;
                    case UVM32_SYSCALL_PRINT: {
                        const char *str = uvm32_arg_getcstr(vmst, &evt, ARG0);
                        printf("%s", str);
                    } break;
                    case UVM32_SYSCALL_PRINTLN: {
                        const char *str = uvm32_arg_getcstr(vmst, &evt, ARG0);
                        printf("%s\n", str);
                    } break;
                    case UVM32_SYSCALL_PRINTDEC:
                        printf("%d", uvm32_arg_getval(vmst, &evt, ARG0));
                    break;
                    case UVM32_SYSCALL_PUTC:
                        printf("%c", uvm32_arg_getval(vmst, &evt, ARG0));
                    break;
                    case UVM32_SYSCALL_PRINTHEX:
                        printf("%08x", uvm32_arg_getval(vmst, &evt, ARG0));
                    break;
                    case UVM32_SYSCALL_MILLIS: {
                        uvm32_arg_setval(vmst, &evt, RET, SDL_GetTicks());
                    } break;
                    case UVM32_SYSCALL_GETC: {
                        uvm32_arg_setval(vmst, &evt, RET, 0xFFFFFFFF);
                    } break;
                    case UVM32_SYSCALL_RENDERAUDIO: {
                        uvm32_slice_t buf = uvm32_arg_getslice(vmst, &evt, ARG0, ARG1);
                        printf("Got audio buf len=%d\n", buf.len);
                        int16_t *samples = (int16_t *)buf.ptr;
                        for (int i=0;i<buf.len/2;i++) {
                            audio_enq(samples[i]);
                        }

                    } break;
                    case UVM32_SYSCALL_RENDER: {
                        uvm32_slice_t buf = uvm32_arg_getslice(vmst, &evt, ARG0, ARG1);

                        // copy into texture
                        void* dst;
                        const uint8_t* src = buf.ptr;
                        int src_pitch = WIDTH * 4;
                        int dst_pitch;
                        if (SDL_LockTexture(render_target, NULL, &dst, &dst_pitch)) {
                            for (int y = 0; y < HEIGHT; y++) {
                                memcpy(dst, src, src_pitch);
                                dst = (uint8_t*)dst + dst_pitch;
                                src += src_pitch;
                            }
                            SDL_UnlockTexture(render_target);
                        }

                        // stretch to window
                        SDL_FRect src_rect = {0, 0, WIDTH, HEIGHT };
                        SDL_FRect dst_rect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
                        SDL_RenderTexture(renderer, render_target, &src_rect, &dst_rect);
                        SDL_RenderPresent(renderer);
                    } break;
                    case UVM32_SYSCALL_GETKEY: {
                        keyevent_t ke;
                        if (key_deq(&ke)) {
                            uint32_t code = (ke.down ? 0x80000000 : 0) | ke.scancode;
                            uvm32_arg_setval(vmst, &evt, RET, code);
                        } else {
                            uvm32_arg_setval(vmst, &evt, RET, 0xFFFFFFFF);
                        }
                    } break;
                    default:
                        printf("Unhandled syscall 0x%08x\n", evt.data.syscall.code);
                    break;
                }
            break;
            default:
                printf("Bad evt %d\n", evt.typ);
                return 1;
            break;
        }
        fflush(stdout);
    }

    printf("Executed total of %lu instructions and %lu syscalls\n", (unsigned long)total_instrs, (unsigned long)num_syscalls);

    free(rom);
    if (extram_buf != NULL) {
        free(extram_buf);
    }

    // put terminal back to how it was
    return 0;
}
