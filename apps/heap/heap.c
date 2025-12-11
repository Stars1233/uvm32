#include "uvm32_target.h"
#include "malloc_freelist.h"

uint32_t* extram = (uint32_t*)UVM32_EXTRAM_BASE;
uint32_t extram_len = HEAP_SIZE;

void* memcpy(void* dst, const void* src, int len) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    while (len--) {
        *(d++) = *(s++);
    }
    return dst;
}

void* memset(void* buf, int c, int len) {
    uint8_t* b = (uint8_t*)buf;
    while (len--) {
        *(b++) = c;
    }
    return buf;
}

void* memmove(void* dest, const void* src, size_t len) {
    char* d = dest;
    const char* s = src;
    if (d < s)
        while (len--)
            *d++ = *s++;
    else {
        const char* lasts = s + (len - 1);
        char* lastd = d + (len - 1);
        while (len--)
            *lastd-- = *lasts--;
    }
    return dest;
}

void force_crash(void) {
    uint8_t *p = (uint8_t *)0;
    p[0] = 0;
}

void main(void) {
    malloc_addblock(extram, extram_len);

    uint8_t* p1 = fl_malloc(128);
    if (p1 == NULL) {
        println("malloc failed");
    } else {
        println("malloc ok");
    }
    memset(p1, 'a', 128);
    fl_free(p1);

    uint8_t* p2 = fl_malloc(256);
    if (p2 == NULL) {
        println("malloc failed");
    } else {
        println("malloc ok");
    }
    memset(p2, 'b', 256);
    fl_free(p2);

    //force_crash();    // allows dump to be inspected
}
