TESTS = \
    basic_syscalls  \
    stackoverflow   \
    custom_syscall  \
    syscall_args \
    meter \
    extram \
    badcode \
    invalid_opcodes

RUNCMD = $(foreach TEST,${TESTS},make -C ${TEST} &&)
CLEANCMD = $(foreach TEST,${TESTS},make -C ${TEST} clean &&)

ifeq (,$(shell which gcovr))
	GCOVRCMD=echo Install gcovr for code coverage reports
else
    GCOVRCMD=gcovr -r ../ --filter ".*uvm32.c" && gcovr -r ../ --filter ".*uvm32.c" --cobertura-pretty --cobertura coverage.xml
endif

all:
	${RUNCMD} true
	@${GCOVRCMD}

clean:
	${CLEANCMD} true
	rm -f coverage.xml

