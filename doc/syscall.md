# Syscalls

## Introduction

To interact with the host, code running in the vm makes syscalls.
A syscall acts like a function call which accepts up to two arguments and optionally returns one.

From [apps/common/uvm32_target.h](apps/common/uvm32_target.h):

```c
uint32_t syscall(uint32_t id, uint32_t param1, uint32_t param2) {
...
}
```

Both arguments and the return type are `uint32_t` meaning they are large enough (on uvm32 - a 32bit machine) to hold any type of data.

To do anything useful, VM code must make syscalls and you will likely need to define some which make sense for your application.

(**Note**, uvm32 assumes that hosts are the same endianness as the vm - little endian. If this is not the case, you will have a bad time, patches welcome...)

## Inbuilt systems

There are two inbuilt syscalls used by uvm32, `halt()` and `yield()`.

`halt()` tells the host that the program has ended normally. `yield()` tells the host that the program requires more instructions to be executed. Halt is handled internally and transitions the VM to `UVM32_STATUS_ENDED`, `yield()` is handled in the VM host like other syscalls. 

## Worked example

[`common/uvm32_common_custom.h`](common/uvm32_common_custom.h) defines numbers for a few useful syscalls, for example we have a syscall which prints a single NULL terminated C string:

```c
#define UVM32_SYSCALL_PRINT    0x00000002
```

Using this definition, we can now use the syscall from vm code and handle it in the host.

In the vm code, print "Hello, world\n"

```c
syscall(UVM32_SYSCALL_PRINT, (uint32_t)"Hello world\n", 0);
```

The host code, we will receive an event for every syscall. We receive and handle the syscall as follows:

```c
uvm32_state_t vmst;
uvm32_evt_t evt;
uvm32_init(&vmst);
uvm32_load(&vmst, code, code_len);
uvm32_run(&vmst, &evt, 100);
switch(evt.typ) {
    case UVM32_EVT_SYSCALL:
        switch(evt.data.syscall.code) {
            case UVM32_SYSCALL_PRINT:
                printf("%s", uvm32_arg_getcstr(&vmst, &evt, ARG0));
            break;
        }
    break;
    ...
}
```

In order to get the string we expect in argument 0 (the first argument), we call `uvm32_arg_getcstr(&vmst, &evt, ARG0)`.

## Syscall argument handling

You might reasonably ask, "why can't I just get the string pointer? why must I call `uvm32_arg_getval()`?".

The answer is safety. uvm32 takes the view that code running inside the sandbox is untrusted and so could send all kind of invalid data in syscalls. Imagine what might happens if we did:

```c
syscall(UVM32_SYSCALL_PRINT, (uint32_t)0xBADBAD, 0);
```

Our host code could receive anything. Perhaps there is a valid C string in that location, or perhaps there is data which never terminates, leaving the host trying forever to print the string.

uvm32 guarantees that syscall arguments are safe to access in the host. To do this, it places some limits on what can be passed through a syscall. All arguments and the return type must be accessed through the `uvm32_arg_*` set of functions

## `uvm32_arg_getval()`

`uint32_t uvm32_arg_getval(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t arg)`

Reads either `ARG0` or `ARG1` and returns the value as a `uint32_t`.

Passing other integer types requires both sides to cast appropriately, for example:

```c
int16_t x = -1234;
syscall(UVM32_SYSCALL_PRINTI16, (uint32_t)int16_t, 0);
```

```c
case UVM32_EVT_SYSCALL:
    switch(evt.data.syscall.code) {
        case UVM32_SYSCALL_PRINTI16:
            printf("%d", (int16_t)uvm32_arg_getval(&vmst, &evt, ARG0));
        break;
    }
break;
```

## `uvm32_arg_getcstr()`

`const char *uvm32_arg_getcstr(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t arg);`

Reads either `ARG0` or `ARG1` and returns the value as a terminated C string in valid memory for the host. To achieve this and guarantee safety, uvm32 will check that every byte including the NULL terminator are safe to access. If the string is invalid and would lead to reading outside of the vm's memory space, an empty string (not a NULL will be returned) and the next call to `uvm32_run()` will pass back `UVM32_EVT_ERR`.

Though convenient, `uvm32_arg_getcstr()` in inefficient as it must scan the entire string to check it is safe to access.

## `uvm32_arg_getslice()`

`uvm32_slice_t uvm32_arg_getslice(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_a
rg_t argPtr, uvm32_arg_t argLen);`

Reads a slice (a bounded array) of memory where `argPtr` holds the starting address and `argLen` holds the length.

For example, in the VM:

```c
uint8_t buf[10];
for (int i=0;i<10;i++) {
    buf[i] = i*10;
}
syscall(PRINTBUF, (uint32_t)buf, sizeof(buf));
```

In the host:

```c
uvm32_slice_t slice = uvm32_arg_getslice(vmst, evt, ARG0, ARG1);
for (int i=0;i<slice.len;i++) {
    printf("%d\n", slice.ptr[i]);
}
```

As the lenth is known in advance, `uvm32_arg_getslice()` is both fast and safe.

## `uvm32_arg_getslice_fixed()`

Where the host knows in advance what size slice to expect, it can use:

`uvm32_slice_t uvm32_arg_getslice_fixed(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t arg, uint32_t len)`

For example, in the VM:

```c
float vector3_pos[3] = {0.5f, 10.0f, 13.2f);
syscall(PRINTVEC3, (uint32_t)vector3_pos, 0);
```

In the host:

```c
uvm32_slice_t slice = uvm32_arg_getslice_fixed(vmst, evt, ARG0, sizeof(float) * 3);
float *vector3 = (float *)slice.ptr;
...

## `uvm32_arg_setval`

`void uvm32_arg_setval(uvm32_state_t *vmst, uvm32_evt_t *evt, uvm32_arg_t, uint3
2_t val);`

Writes a `uint32_t` value into the pointer in the argument. Other integer types can be handled through casting.

For example, in the VM:

```c
int8_t val;
syscall(GETI8, (uint32_t)&val, 0);
// val now equals -73
```

In the host:
```c
int8_t x = -73;
uvm32_arg_setval(vmst, evt, ARG0, (uint32)x)
```

## Returning values

Currently, uvm32 only supports returning integer types from syscalls, as returning a bare pointer into the host's memory space is unsafe.

To return a value from a syscall, in the VM:

```c
uint32_t sum = syscall(ADD, 13, 17);
...
```

In the host:

```
uint32_t a = uvm32_arg_getval(vmst, evt, ARG0);
uint32_t b = uvm32_arg_getval(vmst, evt, ARG1);
uvm32_arg_setval(vmst, evt, RET, a+b);

```

# syscall ABI

To make a syscall, register `a7` is set with the syscall number (a `UVM32_SYSCALL_x`) and `a0`, `a1` are set with the syscall parameters. The response is returned in `a2`.

[target.h](common/uvm32_target.h#L12)

```c
static uint32_t syscall(uint32_t id, uint32_t param1, uint32_t param2) {
    register uint32_t a0 asm("a0") = (uint32_t)(param1);
    register uint32_t a1 asm("a1") = (uint32_t)(param2);
    register uint32_t a2 asm("a2");
    register uint32_t a7 asm("a7") = (uint32_t)(id);

    asm volatile (
        "ecall"
        : "=r"(a2) // output
        : "r"(a7), "r"(a0), "r"(a1) // input
        : "memory"
    );
    return a2;
}
```
The [RISC-V SBI](https://github.com/riscv-non-isa/riscv-sbi-doc/blob/master/riscv-sbi.adoc) is not followed, a simpler approach is taken.


