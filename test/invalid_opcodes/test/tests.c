#include <string.h>
#include "unity.h"
#include "uvm32.h"
#include "../common/uvm32_common_custom.h"

#include "rom-header.h"

static uvm32_state_t vmst;
static uvm32_evt_t evt;

void setUp(void) {
}

void tearDown(void) {
}

void test_invalid_opcode_rd_extram(void) {
    // https://www.cs.sfu.ca/~ashriram/Courses/CS295/assets/notebooks/RISCV/RISCV_CARD.pdf
    // lb,lbu etc only have funct3 values of 0,1,2,4,5
    uint8_t bad_funct3_3[] = {
        0xb7, 0x0f, 0x00, 0x10, // lui t6,0x10000
        0x83, 0xB2, 0x0f, 0x00  // l? t0,0(t6)
    };

    uvm32_init(&vmst);
    uvm32_load(&vmst, bad_funct3_3, 8);
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_ERR);
    TEST_ASSERT_EQUAL(evt.data.err.errcode, UVM32_ERR_INTERNAL_CORE);

    uint8_t bad_funct3_6[] = {
        0xb7, 0x0f, 0x00, 0x10, // lui t6,0x10000
        0x83, 0xE2, 0x0f, 0x00  // l? t0,0(t6)
    };

    uvm32_init(&vmst);
    uvm32_load(&vmst, bad_funct3_6, 8);
    uvm32_run(&vmst, &evt, 100);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_ERR);
    TEST_ASSERT_EQUAL(evt.data.err.errcode, UVM32_ERR_INTERNAL_CORE);
}


