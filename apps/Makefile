DOCKER_IMAGE=riscv-dev

all:
	docker build -t ${DOCKER_IMAGE} .
	(cd sketch && make)
	(cd helloworld && make)
	(cd zig-mandel && make)
	(cd zigtris && make)
	(cd rust-hello && make)
	(cd hello-asm && make)

clean:
	(cd sketch && make clean)
	(cd helloworld && make clean)
	(cd zig-mandel && make clean)
	(cd zigtris && make clean)
	(cd rust-hello && make clean)
	(cd hello-asm && make clean)

