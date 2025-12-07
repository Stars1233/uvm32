const std = @import("std");
const CrossTarget = @import("std").zig.CrossTarget;
const Target = @import("std").Target;
const Feature = @import("std").Target.Cpu.Feature;

pub fn build(b: *std.Build) void {
    const features = Target.riscv.Feature;
    var disabled_features = Feature.Set.empty;
    var enabled_features = Feature.Set.empty;

    // disable all CPU extensions
    disabled_features.addFeature(@intFromEnum(features.a));
    disabled_features.addFeature(@intFromEnum(features.c));
    disabled_features.addFeature(@intFromEnum(features.d));
    disabled_features.addFeature(@intFromEnum(features.e));
    disabled_features.addFeature(@intFromEnum(features.f));
    // except multiply
    enabled_features.addFeature(@intFromEnum(features.m));

    const target = b.resolveTargetQuery(.{
        .cpu_arch = Target.Cpu.Arch.riscv32,
        .os_tag = Target.Os.Tag.freestanding,
        .abi = Target.Abi.none,
        .cpu_model = .{ .explicit = &std.Target.riscv.cpu.generic_rv32},
        .cpu_features_sub = disabled_features,
        .cpu_features_add = enabled_features
    });

    const exe = b.addExecutable(.{
        .name = "zigtris",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = .ReleaseSmall,
        }),
    });

    // add mibu for zigtris event generation
    const mibu = b.dependency("mibu", .{
        .target = target,
        .optimize = .ReleaseSmall,
    });
    exe.root_module.addImport("mibu", mibu.module("mibu"));

    const zigtris_dep = b.dependency("zigtris", .{
        .target = target,
        .optimize = .ReleaseSmall,
    });

    const zigtris_mod = zigtris_dep.module("zigtris");
    exe.root_module.addImport("zigtris", zigtris_mod);

    b.installArtifact(exe);

    exe.addAssemblyFile(b.path("../crt0.s"));
    exe.setLinkerScript(b.path("../linker.ld"));
    exe.addIncludePath(b.path("../../common"));

    const bin = b.addObjCopy(exe.getEmittedBin(), .{
        .format = .bin,
    });
    bin.step.dependOn(&exe.step);

    const copy_bin = b.addInstallBinFile(bin.getOutput(), "zigtris.bin");
    b.default_step.dependOn(&copy_bin.step);
}
