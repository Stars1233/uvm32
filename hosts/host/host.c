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

#include "../common/uvm32_common_custom.h"

// stash terminal settings on startup
static struct termios orig_termios;

void disableRawMode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("\033[?25h");    // show cursor
    fflush(stdout);
}

void cleanexit(int sig) {
    disableRawMode();
    exit(0);
}

void enableRawMode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    signal(SIGINT, cleanexit);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_cflag |= (CS8);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
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

// Poll for input, inelegant, but works and doesn't confuse the main
// flow of the program with select/poll
bool poll_getch(uint8_t* c) {
    struct timeval tv;
    fd_set readfds;
    int ret;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

    if (ret == -1) {
        printf("console read failed\r\n");
        exit(1);
    } else if (!ret) {
        // timeout
        return false;
    }

    if (FD_ISSET(STDIN_FILENO, &readfds)) {
        int len;

        len = read(STDIN_FILENO, c, 1);
        if (len == -1) {
            printf("console read failed\r\n");
            exit(1);
        }

        if (len) {
            if (*c == 0x03 || *c == 0x04) {   // ctrl-c ctrl-d
                disableRawMode();
                exit(0);
            }
            if (*c == 0x0d) {
                *c = '\n';
            }
            return true;
        }
    }
    return false;
}

void hexdump(const uint8_t *p, int len) {
    while(len--) {
        printf("%02x", *p++);
    }
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

int main(int argc, char *argv[]) {
    uvm32_state_t vmst;
    uint32_t max_instrs_per_run = 500000;
    clock_t start_time = clock() / (CLOCKS_PER_SEC / 1000);
    char c;
    const char *rom_filename = NULL;
    uint32_t extram_len = 0;
    uint32_t *extram_buf = NULL;
    uvm32_evt_t evt;
    bool isrunning = true;
    uint32_t total_instrs = 0;
    uint32_t num_syscalls = 0;
    int romlen = 0;

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
        uvm32_extram(&vmst, extram_buf, extram_len);
    }

    // setup terminal for getch()
    enableRawMode();

    while(isrunning) {
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
                        uint8_t c;
                        if (poll_getch(&c)) {
                            uvm32_setval(&vmst, &evt, RET, c);
                        } else {
                            uvm32_setval(&vmst, &evt, RET, 0xFFFFFFFF);
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

    printf("Executed total of %d instructions and %d syscalls\n", (int)total_instrs, (int)num_syscalls);

    free(rom);
    if (extram_buf != NULL) {
        free(extram_buf);
    }

    // put terminal back to how it was
    disableRawMode();
    return 0;
}
