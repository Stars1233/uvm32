#include <string.h>
#include "unity.h"
#include "uvm32.h"
#include "../common/uvm32_common_custom.h"

#include "rom-header.h"

static uvm32_state_t vmst;
static uvm32_evt_t evt;

void setUp(void) {
    // runs before each test
    uvm32_init(&vmst);
    uvm32_load(&vmst, rom_bin, rom_bin_len);
}

void tearDown(void) {
}

// run program until printdec() reaches 100
// every time it hangs, resume
// run in batches of num_instr
uint32_t metered_run(uint32_t num_instr) {
    uint32_t expected = 0;
    uint32_t total_instr = 0;

    while(expected < 100) {
        total_instr += uvm32_run(&vmst, &evt, num_instr);
        switch(evt.typ) {
            case UVM32_EVT_SYSCALL: {
                TEST_ASSERT_EQUAL(UVM32_SYSCALL_PRINTDEC, evt.data.syscall.code);
                uint32_t val = uvm32_arg_getval(&vmst, &evt, ARG0);
                TEST_ASSERT_EQUAL(val, expected);
                expected++;
            } break;
            case UVM32_EVT_ERR:
                TEST_ASSERT_EQUAL(evt.data.err.errcode, UVM32_ERR_HUNG);
                uvm32_clearError(&vmst);    // clear the hung error so it can continue
            break;
            case UVM32_EVT_END:
                TEST_ASSERT_EQUAL(0, 1);    // trigger an assert, we didn't get to 100000 yet
            break;
            case UVM32_EVT_BREAK:
            break;
        }
    }

    // run vm to completion
    total_instr += uvm32_run(&vmst, &evt, 10000);
    TEST_ASSERT_EQUAL(evt.typ, UVM32_EVT_END);
    return total_instr;
}

void test_meter(void) {
    // test many different meter values, including 0 (which should run 1 instruction, as 0 is going to hang)
    uint32_t total_instr = 0;

    for (uint32_t i=0;i<1000;i++) {
        uvm32_init(&vmst);
        uvm32_load(&vmst, rom_bin, rom_bin_len);
        uint32_t instrs = metered_run(i);
        if (total_instr == 0) {    // first run
            total_instr = instrs;
        } else {
            TEST_ASSERT_EQUAL(total_instr, instrs); // check that every run takes the total number of instructions
        }
    }
}

