const uvm = @import("uvm.zig");
const zeptolibc = @import("zeptolibc");
const std = @import("std");

const console = @import("console.zig").getWriter();
const pd = @cImport({
    @cInclude("puredoom/PureDOOM.h");
});
const sdlkeys = @cImport({
    @cInclude("SDL_scancode.h");
});

const wad_data = @embedFile("doom1.wad");

const WIDTH = 320;
const HEIGHT = 200;

const WAD_FILE_HANDLE: *c_int = @ptrFromInt(0x00000008); // some unique value we give when wad file opened
var wad_stream_offset: usize = 0;

fn consoleWriteFn(data:[]const u8) void {
    _ = console.print("{s}", .{data}) catch 0;
    _ = console.flush() catch 0;
}

// implement a backend for puredoom
export fn doom_print_impl(msg: [*:0]const u8) callconv(.c) void {
    _ = console.print("{s}", .{std.mem.span(msg)}) catch 0;
    _ = console.flush() catch 0;
}

export fn doom_exit_impl(code: c_int) void {
    _ = code;
    uvm.halt();
}

export fn doom_gettime_impl(sec: *c_int, usec: *c_int) callconv(.c) void {
    const now = uvm.millis();
    sec.* = @intCast(now / 1000);
    usec.* = @intCast(@mod(now * 1000, 1000000));
}

export fn doom_open_impl(filename: [*:0]const u8, mode: [*]const u8) callconv(.c) ?*c_int {
    _ = mode;
    _ = console.print("doom_open_impl {s}\n", .{std.mem.span(filename)}) catch 0;
    _ = console.flush() catch 0;
    if (std.mem.eql(u8, std.mem.span(filename), "/doom1.wad")) {
        return WAD_FILE_HANDLE;
    }
    return null;
}
export fn doom_close_impl(handle: *anyopaque) callconv(.c) void {
    // don't care, all files are in memory
    _ = handle;
}
export fn doom_read_impl(handle: *c_int, buf: [*]u8, count: c_int) callconv(.c) c_int {
    if (handle != WAD_FILE_HANDLE) {
        _ = console.print("doom_read_impl invalid handle!\n", .{}) catch 0;
        _ = console.flush() catch 0;
        return 0;
    }
    const dst = buf[0..@intCast(count)];
    const src = wad_data[wad_stream_offset .. wad_stream_offset + @as(usize, @intCast(count))];

    std.mem.copyForwards(u8, dst, src);
    wad_stream_offset += @intCast(count);
    return count;
}
export fn doom_write_impl(handle: *c_int, buf: *const anyopaque, count: c_int) callconv(.c) c_int {
    _ = handle;
    _ = buf;
    _ = count;
    _ = console.print("doom_write_impl unsupported!\n", .{}) catch 0;
    _ = console.flush() catch 0;
    return 0;
}
export fn doom_seek_impl(handle: *c_int, offset: c_int, origin: pd.doom_seek_t) callconv(.c) c_int {
    if (handle != WAD_FILE_HANDLE) {
        _ = console.print("doom_seek_impl invalid handle!\n", .{}) catch 0;
        _ = console.flush() catch 0;
        return 0;
    }
    switch (origin) {
        pd.DOOM_SEEK_CUR => wad_stream_offset += @intCast(offset),
        pd.DOOM_SEEK_END => wad_stream_offset = wad_data.len - @as(usize, @intCast(offset)),
        pd.DOOM_SEEK_SET => wad_stream_offset = @as(usize, @intCast(offset)),
        else => {},
    }
    return 0;
}
export fn doom_tell_impl(handle: *c_int) callconv(.c) c_int {
    if (handle != WAD_FILE_HANDLE) {
        _ = console.print("doom_tell_impl invalid handle!\n", .{}) catch 0;
        _ = console.flush() catch 0;
        return 0;
    }
    return @intCast(wad_stream_offset);
}
export fn doom_eof_impl(handle: *c_int) callconv(.c) c_int {
    if (handle != WAD_FILE_HANDLE) {
        _ = console.print("doom_eof_impl invalid handle!\n", .{}) catch 0;
        _ = console.flush() catch 0;
        return 1;
    }
    if (wad_stream_offset >= wad_data.len) {
        return 1;
    } else {
        return 0;
    }
}

fn scancode_to_doom_key(scancode:u16) pd.doom_key_t {
    return switch(scancode) {
        sdlkeys.SDL_SCANCODE_TAB => pd.DOOM_KEY_TAB,
        sdlkeys.SDL_SCANCODE_RETURN => pd.DOOM_KEY_ENTER,
        sdlkeys.SDL_SCANCODE_ESCAPE => pd.DOOM_KEY_ESCAPE,
        sdlkeys.SDL_SCANCODE_SPACE => pd.DOOM_KEY_SPACE,
        sdlkeys.SDL_SCANCODE_APOSTROPHE => pd.DOOM_KEY_APOSTROPHE,
        sdlkeys.SDL_SCANCODE_KP_MULTIPLY => pd.DOOM_KEY_MULTIPLY,
        sdlkeys.SDL_SCANCODE_COMMA => pd.DOOM_KEY_COMMA,
        sdlkeys.SDL_SCANCODE_MINUS => pd.DOOM_KEY_MINUS,
        sdlkeys.SDL_SCANCODE_PERIOD => pd.DOOM_KEY_PERIOD,
        sdlkeys.SDL_SCANCODE_SLASH => pd.DOOM_KEY_SLASH,
        sdlkeys.SDL_SCANCODE_0 => pd.DOOM_KEY_0,
        sdlkeys.SDL_SCANCODE_1 => pd.DOOM_KEY_1,
        sdlkeys.SDL_SCANCODE_2 => pd.DOOM_KEY_2,
        sdlkeys.SDL_SCANCODE_3 => pd.DOOM_KEY_3,
        sdlkeys.SDL_SCANCODE_4 => pd.DOOM_KEY_4,
        sdlkeys.SDL_SCANCODE_5 => pd.DOOM_KEY_5,
        sdlkeys.SDL_SCANCODE_6 => pd.DOOM_KEY_6,
        sdlkeys.SDL_SCANCODE_7 => pd.DOOM_KEY_7,
        sdlkeys.SDL_SCANCODE_8 => pd.DOOM_KEY_8,
        sdlkeys.SDL_SCANCODE_9 => pd.DOOM_KEY_9,
        sdlkeys.SDL_SCANCODE_SEMICOLON => pd.DOOM_KEY_SEMICOLON,
        sdlkeys.SDL_SCANCODE_EQUALS => pd.DOOM_KEY_EQUALS,
        sdlkeys.SDL_SCANCODE_LEFTBRACKET => pd.DOOM_KEY_LEFT_BRACKET,
        sdlkeys.SDL_SCANCODE_RIGHTBRACKET => pd.DOOM_KEY_RIGHT_BRACKET,
        sdlkeys.SDL_SCANCODE_A => pd.DOOM_KEY_A,
        sdlkeys.SDL_SCANCODE_B => pd.DOOM_KEY_B,
        sdlkeys.SDL_SCANCODE_C => pd.DOOM_KEY_C,
        sdlkeys.SDL_SCANCODE_D => pd.DOOM_KEY_D,
        sdlkeys.SDL_SCANCODE_E => pd.DOOM_KEY_E,
        sdlkeys.SDL_SCANCODE_F => pd.DOOM_KEY_F,
        sdlkeys.SDL_SCANCODE_G => pd.DOOM_KEY_G,
        sdlkeys.SDL_SCANCODE_H => pd.DOOM_KEY_H,
        sdlkeys.SDL_SCANCODE_I => pd.DOOM_KEY_I,
        sdlkeys.SDL_SCANCODE_J => pd.DOOM_KEY_J,
        sdlkeys.SDL_SCANCODE_K => pd.DOOM_KEY_K,
        sdlkeys.SDL_SCANCODE_L => pd.DOOM_KEY_L,
        sdlkeys.SDL_SCANCODE_M => pd.DOOM_KEY_M,
        sdlkeys.SDL_SCANCODE_N => pd.DOOM_KEY_N,
        sdlkeys.SDL_SCANCODE_O => pd.DOOM_KEY_O,
        sdlkeys.SDL_SCANCODE_P => pd.DOOM_KEY_P,
        sdlkeys.SDL_SCANCODE_Q => pd.DOOM_KEY_Q,
        sdlkeys.SDL_SCANCODE_R => pd.DOOM_KEY_R,
        sdlkeys.SDL_SCANCODE_S => pd.DOOM_KEY_S,
        sdlkeys.SDL_SCANCODE_T => pd.DOOM_KEY_T,
        sdlkeys.SDL_SCANCODE_U => pd.DOOM_KEY_U,
        sdlkeys.SDL_SCANCODE_V => pd.DOOM_KEY_V,
        sdlkeys.SDL_SCANCODE_W => pd.DOOM_KEY_W,
        sdlkeys.SDL_SCANCODE_X => pd.DOOM_KEY_X,
        sdlkeys.SDL_SCANCODE_Y => pd.DOOM_KEY_Y,
        sdlkeys.SDL_SCANCODE_Z => pd.DOOM_KEY_Z,
        sdlkeys.SDL_SCANCODE_BACKSPACE => pd.DOOM_KEY_BACKSPACE,
        sdlkeys.SDL_SCANCODE_LCTRL => pd.DOOM_KEY_CTRL,
        sdlkeys.SDL_SCANCODE_RCTRL => pd.DOOM_KEY_CTRL,
        sdlkeys.SDL_SCANCODE_LEFT => pd.DOOM_KEY_LEFT_ARROW,
        sdlkeys.SDL_SCANCODE_UP => pd.DOOM_KEY_UP_ARROW,
        sdlkeys.SDL_SCANCODE_RIGHT => pd.DOOM_KEY_RIGHT_ARROW,
        sdlkeys.SDL_SCANCODE_DOWN => pd.DOOM_KEY_DOWN_ARROW,
        sdlkeys.SDL_SCANCODE_RSHIFT => pd.DOOM_KEY_SHIFT,
        sdlkeys.SDL_SCANCODE_LSHIFT => pd.DOOM_KEY_SHIFT,
        sdlkeys.SDL_SCANCODE_LALT => pd.DOOM_KEY_ALT,
        sdlkeys.SDL_SCANCODE_RALT => pd.DOOM_KEY_ALT,
        sdlkeys.SDL_SCANCODE_F1 => pd.DOOM_KEY_F1,
        sdlkeys.SDL_SCANCODE_F2 => pd.DOOM_KEY_F2,
        sdlkeys.SDL_SCANCODE_F3 => pd.DOOM_KEY_F3,
        sdlkeys.SDL_SCANCODE_F4 => pd.DOOM_KEY_F4,
        sdlkeys.SDL_SCANCODE_F5 => pd.DOOM_KEY_F5,
        sdlkeys.SDL_SCANCODE_F6 => pd.DOOM_KEY_F6,
        sdlkeys.SDL_SCANCODE_F7 => pd.DOOM_KEY_F7,
        sdlkeys.SDL_SCANCODE_F8 => pd.DOOM_KEY_F8,
        sdlkeys.SDL_SCANCODE_F9 => pd.DOOM_KEY_F9,
        sdlkeys.SDL_SCANCODE_F10 => pd.DOOM_KEY_F10,
        sdlkeys.SDL_SCANCODE_F11 => pd.DOOM_KEY_F11,
        sdlkeys.SDL_SCANCODE_F12 => pd.DOOM_KEY_F12,
        sdlkeys.SDL_SCANCODE_PAUSE => pd.DOOM_KEY_PAUSE,
        else => pd.DOOM_KEY_UNKNOWN,
    };
}

fn submain() !void {
    // init zepto with a memory allocator and console writer
    zeptolibc.init(uvm.allocator(), consoleWriteFn);

    pd.doom_set_resolution(WIDTH, HEIGHT);
    pd.pd_init();

    while(true) {
        pd.doom_update();
        const doomSndBuf: [*]i16 = pd.doom_get_sound_buffer();
        uvm.renderAudio(doomSndBuf, 2048);

        const fb: [*]const u8 = pd.doom_get_framebuffer(4);
        uvm.render(fb, WIDTH * HEIGHT * 4);

        var pressed:bool = undefined;
        var scancode:u16 = undefined;
        if (uvm.getkey(&scancode, &pressed)) {
            if (pressed) {
                pd.doom_key_down(scancode_to_doom_key(scancode));
            } else {
                pd.doom_key_up(scancode_to_doom_key(scancode));
            }
        }
    }
}

export fn main() void {
    _ = submain() catch {
        uvm.println("Caught err");
    };
}
