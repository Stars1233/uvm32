TOPDIR=../..
PROJECT:=$(shell basename ${PWD})

HEAP_SIZE=$(shell echo "1024 * 512" | bc)
OPT=-Os
CFLAGS=-DHEAP_SIZE=${HEAP_SIZE}
SRCS=${PROJECT}.c ${TOPDIR}/apps/crt0.S malloc_freelist.c
HOST_EXTRA=-e ${HEAP_SIZE} -i 100000
all: all_common
test: test_common
clean: clean_common
include ${TOPDIR}/apps/makefile.common
