#include <string.h>
#include "unity.h"
#include "uvm32.h"
#include "../common/uvm32_common_custom.h"

#include "rom-header.h"

static uvm32_state_t vmst;
//static uvm32_evt_t evt;

void setUp(void) {
}

void tearDown(void) {
}

void test_giant_rom(void) {
    // try to load a ROM bigger than we have space for
    uvm32_init(&vmst);
    TEST_ASSERT_EQUAL(false, uvm32_load(&vmst, rom_bin, UVM32_MEMORY_SIZE+1)); // if it reads off end of rom_bin, will crash
}

void test_negative_rom_length(void) {
    uvm32_init(&vmst);
    TEST_ASSERT_EQUAL(false, uvm32_load(&vmst, rom_bin, -1));
}

