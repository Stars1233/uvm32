#include "uvm32_target.h"
#include "shared.h"

void main(void) {
    switch(syscall(SYSCALL_PICKTEST, 0, 0)) {
        case TEST1:
            asm("auipc t0, 0"); // copy pc into t0
            asm("auipc t1, 0"); // copy pc into t1
        break;
    }
}

