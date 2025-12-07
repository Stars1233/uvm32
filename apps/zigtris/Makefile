all:
	# zig's objcopy is broken, so use external tool
	# https://ziggit.dev/t/addobjcopy-producing-zero-padding-at-start-of-binary/13384
	zig build && docker run -v `pwd`:/data -w /data --rm riscv-dev riscv64-unknown-elf-objcopy zig-out/bin/zigtris -O binary zigtris.bin

clean:
	rm -rf zigtris.bin zig-out .zig-cache

test: all
	../../host/host zigtris.bin


