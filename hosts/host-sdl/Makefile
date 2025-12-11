TOPDIR=../..

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
LIBS = ${SDL} -lm -lSDL3
else
LIBS = `pkg-config sdl3 --libs --static`
CFLAGS += `pkg-config sdl3 --cflags`
endif

#CFLAGS += -Wall -Werror
CFLAGS += -pedantic -std=c99 -O3
CFLAGS += -DUVM32_MEMORY_SIZE=65536

all:
	gcc ${CFLAGS} -I${TOPDIR}/uvm32 -I${TOPDIR}/common -o host-sdl ${TOPDIR}/uvm32/uvm32.c host-sdl.c ${LIBS}

clean:
	rm -f host-sdl
