#include "uvm32_target.h"

uint32_t *framebuffer = (uint32_t *)UVM32_EXTRAM_BASE;
#define WIDTH 800
#define HEIGHT 600

void main(void) {
    uint32_t col = 0x000000FF;
    uint8_t c = 0;
    uint32_t framecount = 0;
    while(1) {
        for (int y=0;y<HEIGHT;y++) {
            for (int x=0;x<WIDTH;x++) {
                framebuffer[y*WIDTH+x] = col;
            }
        }
        col = (c << 24) | (c << 16) | (c<<8) | 0xFF;
        c++;
        printdec(framecount++);
        println("");
    }
}
