const uvm32 = @cImport({
    @cDefine("USE_MAIN", "1");
    @cInclude("uvm32_target.h");
});
const buildopts = @import("buildopts");
const std = @import("std");

const extram:[*]u8 = @ptrFromInt(uvm32.UVM32_EXTRAM_BASE);
const extram_len = buildopts.heapsize;

var fba:std.heap.FixedBufferAllocator = .init(extram[0..extram_len]);

pub fn allocator() std.mem.Allocator {
    return fba.allocator();
}

pub inline fn syscall(id: u32, param1: u32, param2: u32) u32 {
    var val: u32 = undefined;
    asm volatile ("ecall"
        : [val] "={a2}" (val),
        : [param1] "{a0}" (param1), [param2] "{a1}" (param2),
          [id] "{a7}" (id),
        : .{ .memory = true });
    return val;
}

pub inline fn renderAudio(audbuf: [*]const i16, len:u32) void {
    _ = syscall(uvm32.UVM32_SYSCALL_RENDERAUDIO, @intFromPtr(audbuf), len);
}

pub inline fn render(fb: [*]const u8, len:u32) void {
    _ = syscall(uvm32.UVM32_SYSCALL_RENDER, @intFromPtr(fb), len);
}

pub inline fn getc() ?u8 {
    const key = syscall(uvm32.UVM32_SYSCALL_GETC, 0, 0);
    if (key == 0xFFFFFFFF) {
        return null;
    } else {
        return @truncate(key);
    }
}

pub inline fn getkey(code: *u16, pressed:*bool) bool {
    const k = syscall(uvm32.UVM32_SYSCALL_GETKEY, 0, 0);
    if (k == 0xFFFFFFFF) {
        return false;
    } else {
        code.* = @truncate(k & 0xFFFF);
        pressed.* = (k & 0x80000000) != 0;
        return true;
    }
}

pub inline fn millis() u32 {
    return syscall(uvm32.UVM32_SYSCALL_MILLIS, 0, 0);
}

// dupeZ would be better, but want to avoid using an allocator
// this is of course, unsafe...
var termination_buf:[512]u8 = undefined;

pub inline fn print(m: []const u8) void {
    @memcpy(termination_buf[0..m.len], m);
    termination_buf[m.len] = 0;
    const s = termination_buf[0..m.len :0];
    _ = syscall(uvm32.UVM32_SYSCALL_PRINT, @intFromPtr(s.ptr), 0);
}

pub inline fn println(m: []const u8) void {
    @memcpy(termination_buf[0..m.len], m);
    termination_buf[m.len] = 0;
    const s = termination_buf[0..m.len :0];
    _ = syscall(uvm32.UVM32_SYSCALL_PRINTLN, @intFromPtr(s.ptr), 0);
}

pub inline fn yield() void {
    _ = syscall(uvm32.UVM32_SYSCALL_YIELD, 0, 0);
}

pub inline fn halt() void {
    _ = syscall(uvm32.UVM32_SYSCALL_HALT, 0, 0);
}

pub inline fn putc(c:u8) void {
    _ = syscall(uvm32.UVM32_SYSCALL_PUTC, c, 0);
}

