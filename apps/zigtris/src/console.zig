const std = @import("std");
extern fn console_write(data: [*]const u8, len: usize) void;
var wbuf:[4096]u8 = undefined;
var cw = ConsoleWriter.init(&wbuf);
const uvm = @import("uvm.zig");

pub const WriteError = error{ Unsupported, NotConnected };

pub const ConsoleWriter = struct {
    interface: std.Io.Writer,
    err: ?WriteError = null,

    fn drain(w: *std.Io.Writer, data: []const []const u8, splat: usize) std.Io.Writer.Error!usize {
        var ret: usize = 0;

        const b = w.buffered();
        uvm.print(b);
        _ = w.consume(b.len);

        for (data) |d| {
            uvm.print(d);
            ret += d.len;
        }

        const pattern = data[data.len - 1];
        for (0..splat) |_| {
            uvm.print(pattern);
            ret += pattern.len;
        }

        return ret;
    }

    pub fn init(buf: []u8) ConsoleWriter {
        return ConsoleWriter{
            .interface = .{
                .buffer = buf,
                .vtable = &.{
                    .drain = drain,
                },
            },
        };
    }
};

pub fn getWriter() *std.Io.Writer {
    return &cw.interface;
}

