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

#define WIDTH 800
#define HEIGHT 600

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
    exit(1);
}



int main(int argc, char *argv[]) {
    uvm32_state_t vmst;
    uint32_t max_instrs_per_run = 500000;
    clock_t start_time = clock() / (CLOCKS_PER_SEC / 1000);
    char c;
    const char *rom_filename = NULL;
    uint32_t extram_len = WIDTH * HEIGHT * 4;
    uint32_t *extram_buf = NULL;
    uvm32_evt_t evt;
    bool isrunning = true;
    uint32_t total_instrs = 0;
    uint32_t num_syscalls = 0;
    int romlen = 0;
    SDL_Renderer *renderer = NULL;
    SDL_Window *screen = NULL;
    SDL_Event event;
    SDL_Surface *surface = NULL;
    SDL_Texture *tex = NULL;

    // parse commandline args
    while ((c = getopt(argc, argv, "hi:")) != -1) {
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

    uvm32_init(&vmst);

    if (!uvm32_load(&vmst, rom, romlen)) {
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
        uvm32_extram(&vmst, extram_buf, extram_len);
    }

    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        printf("SDL init failed\n");
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);

    screen = SDL_CreateWindow("sdl-host", WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
    if (NULL == screen) {
        printf("SDL CreateWindow failed\n");
        return 1;
    }
    renderer = SDL_CreateRenderer(screen, NULL);
    if (NULL == renderer) {
        printf("SDL CreateRenderer failed\n");
        return 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    surface = SDL_CreateSurfaceFrom(WIDTH, HEIGHT, SDL_PIXELFORMAT_RGBA8888, extram_buf, 4 * WIDTH);
    if (NULL == surface) {
        printf("Failed to create SDL surface\n");
        return 1;
    }


    while (isrunning) {
        SDL_PollEvent(&event);

        switch(event.type) {
            case SDL_EVENT_QUIT:
                exit(0);
            break;
        }

        if (uvm32_extramDirty(&vmst)) {
            tex = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_RenderTexture(renderer, tex, NULL, NULL);
            SDL_DestroyTexture(tex);
            SDL_RenderPresent(renderer);
        }

        total_instrs += uvm32_run(&vmst, &evt, max_instrs_per_run);   // num instructions before vm considered hung
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
                    uvm32_clearError(&vmst);    // allow to continue
                } else {
                    isrunning = false;
                    memdump("host-ram.dump", vmst.memory, UVM32_MEMORY_SIZE);
                    printf("memory dumped to host-ram.dump, pc=0x%08x\n", vmst.core.pc);
                    if (extram_buf != NULL) {
                        memdump("host-extram.dump", (uint8_t *)extram_buf, extram_len);
                        printf("extram dumped to host-extram.dump\n");
                    }
                }
            break;
            case UVM32_EVT_SYSCALL:
                switch(evt.data.syscall.code) {
                    case UVM32_SYSCALL_PRINTBUF: {
                        uvm32_evt_syscall_buf_t buf = uvm32_getbuf(&vmst, &evt, ARG0, ARG1);
                        while(buf.len--) {
                            printf("%02x", *buf.ptr++);
                        }
                    } break;
                    case UVM32_SYSCALL_YIELD: {
                        // uint32_t yield_typ = uvm32_getval(&vmst, &evt, ARG0);
                        // printf("YIELD type=%d\n", yield_typ);
                        // uvm32_setval(&vmst, &evt, RET, 123);
                    } break;
                    case UVM32_SYSCALL_PRINT: {
                        const char *str = uvm32_getcstr(&vmst, &evt, ARG0);
                        printf("%s", str);
                    } break;
                    case UVM32_SYSCALL_PRINTLN: {
                        const char *str = uvm32_getcstr(&vmst, &evt, ARG0);
                        printf("%s\n", str);
                    } break;
                    case UVM32_SYSCALL_PRINTDEC:
                        printf("%d", uvm32_getval(&vmst, &evt, ARG0));
                    break;
                    case UVM32_SYSCALL_PUTC:
                        printf("%c", uvm32_getval(&vmst, &evt, ARG0));
                    break;
                    case UVM32_SYSCALL_PRINTHEX:
                        printf("%08x", uvm32_getval(&vmst, &evt, ARG0));
                    break;
                    case UVM32_SYSCALL_MILLIS: {
                        clock_t now = clock() / (CLOCKS_PER_SEC / 1000);
                        uvm32_setval(&vmst, &evt, RET, now - start_time);
                    } break;
                    case UVM32_SYSCALL_GETC: {
                        uvm32_setval(&vmst, &evt, RET, 0xFFFFFFFF);
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

    printf("Executed total of %d instructions and %d syscalls\n", (int)total_instrs, (int)num_syscalls);

    free(rom);
    if (extram_buf != NULL) {
        free(extram_buf);
    }

    // put terminal back to how it was
    return 0;
}
