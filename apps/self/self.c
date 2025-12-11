#include "uvm32_target.h"
#include "uvm32.h"
#include "../common/uvm32_common_custom.h"
#include "mandel.h"

void main(void) {
    uvm32_state_t vmst;
    uvm32_evt_t evt;
    bool isrunning = true;

    uvm32_init(&vmst);
    uvm32_load(&vmst, mandel, mandel_len);

    while(isrunning) {
        uvm32_run(&vmst, &evt, 100);   // num instructions before vm considered hung

        switch(evt.typ) {
            case UVM32_EVT_END:
                isrunning = false;
            break;
            case UVM32_EVT_SYSCALL:    // vm has paused to handle UVM32_SYSCALL
                switch(evt.data.syscall.code) {
                    case UVM32_SYSCALL_YIELD:
                    break;
                    case UVM32_SYSCALL_PUTC:
                        putc(uvm32_getval(&vmst, &evt, ARG0));
                    break;
                    case UVM32_SYSCALL_PRINTLN: {
                        const char *str = uvm32_getcstr(&vmst, &evt, ARG0);
                        println(str);
                    } break;
                    default:
                        println("Unhandled syscall");
                    break;
                }
            break;
            case UVM32_EVT_ERR:
                println(evt.data.err.errstr);
            break;
            default:
            break;
        }
    }
}

