// Syscall definitions needed by both host and target for sample apps
// These are not required when building a custom host target code with uvm32

// syscalls for exposed host functions, start at 0
#define UVM32_SYSCALL_PUTC        0x00000000
#define UVM32_SYSCALL_GETC        0x00000001
#define UVM32_SYSCALL_PRINT       0x00000002
#define UVM32_SYSCALL_PRINTLN     0x00000003
#define UVM32_SYSCALL_PRINTDEC    0x00000004
#define UVM32_SYSCALL_PRINTHEX    0x00000005
#define UVM32_SYSCALL_MILLIS      0x00000006
#define UVM32_SYSCALL_PRINTBUF    0x00000007
#define UVM32_SYSCALL_RENDER      0x00000008
#define UVM32_SYSCALL_GETKEY      0x00000009
#define UVM32_SYSCALL_RENDERAUDIO 0x0000000A

