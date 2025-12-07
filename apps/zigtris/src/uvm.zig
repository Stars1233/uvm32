const uvm32 = @cImport({
    @cDefine("USE_MAIN", "1");
    @cInclude("uvm32_target.h");
});
const std = @import("std");

// dupeZ would be better, but want to avoid using an allocator
var new_buf:[128]u8 = undefined;
pub inline fn print(m: []const u8) void {
    @memcpy(new_buf[0..m.len], m);
    new_buf[m.len] = 0;
    const s = new_buf[0..m.len :0];

    asm volatile ("csrw " ++ std.fmt.comptimePrint("0x{x}", .{uvm32.IOREQ_PRINT}) ++ ", %[arg1]"
        :
        : [arg1] "r" (s.ptr),
    );
}

pub inline fn println(val: [:0]const u8) void {
    asm volatile ("csrw " ++ std.fmt.comptimePrint("0x{x}", .{uvm32.IOREQ_PRINTLN}) ++ ", %[arg1]"
        :
        : [arg1] "r" (val.ptr),
    );
}

pub inline fn printd(val: u32) void {
    asm volatile ("csrw " ++ std.fmt.comptimePrint("0x{x}", .{uvm32.IOREQ_PRINTD}) ++ ", %[arg1]"
        :
        : [arg1] "r" (val),
    );
}

pub inline fn printx(val: u32) void {
    asm volatile ("csrw " ++ std.fmt.comptimePrint("0x{x}", .{uvm32.IOREQ_PRINTX}) ++ ", %[arg1]"
        :
        : [arg1] "r" (val),
    );
}

pub inline fn printc(val: u32) void {
    asm volatile ("csrw " ++ std.fmt.comptimePrint("0x{x}", .{uvm32.IOREQ_PRINTC}) ++ ", %[arg1]"
        :
        : [arg1] "r" (val),
    );
}

pub inline fn yield() void {
    asm volatile (std.fmt.comptimePrint("csrwi 0x{x}, 0", .{uvm32.IOREQ_YIELD}));
}

var millis_storage:u32 = 0;
pub inline fn millis() u32 {
    asm volatile ("csrw " ++ std.fmt.comptimePrint("0x{x}", .{uvm32.IOREQ_MILLIS}) ++ ", %[arg1]"
        :
        : [arg1] "r" (&millis_storage),
    );
    return millis_storage;
}

var getch_storage:u32 = 0;
pub inline fn getch() ?u8 {
    asm volatile ("csrw " ++ std.fmt.comptimePrint("0x{x}", .{uvm32.IOREQ_GETC}) ++ ", %[arg1]"
        :
        : [arg1] "r" (&getch_storage),
    );
    if (getch_storage <= 0xFF) {
        return @truncate(getch_storage);
    } else {
        return null;
    }
}

