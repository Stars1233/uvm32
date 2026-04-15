TOPDIR=../..

all:
	gcc -Wall -Werror -Wno-error=unused-function -pedantic -std=c99 -O2 -DUVM32_ERROR_STRINGS -DUVM32_MEMORY_SIZE=65536 -DGDBSTUB_ARCH_UVM32 -DGDBSTUB_IMPLEMENTATION -I${TOPDIR}/uvm32 -I${TOPDIR}/common -o host-gdbstub ${TOPDIR}/uvm32/uvm32.c host-gdbstub.c

clean:
	rm -f host-gdbstub
