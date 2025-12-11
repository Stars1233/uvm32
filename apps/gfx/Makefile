TOPDIR=../..
PROJECT:=$(shell basename ${PWD})

OPT=-Os
SRCS=${PROJECT}.c ${TOPDIR}/apps/crt0.S

HOST_EXTRA=-i 4294967295
all: all_common
test: all
	${TOPDIR}/hosts/host-sdl/host-sdl ${HOST_EXTRA} ${PWD}/${PROJECT}.bin

clean: clean_common
include ${TOPDIR}/apps/makefile.common
