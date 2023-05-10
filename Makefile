# Output binary name
bin=miner
lib=libminer.so

# Set the following to '0' to disable log messages:
LOGGER ?= 1

# Compiler/linker flags
CFLAGS += -g -Wall -fPIC -DLOGGER=$(LOGGER)
LDLIBS += -lm
LDFLAGS += -L. -Wl,-rpath='$$ORIGIN'

src=miner.c sha1.c common.c task.c
obj=$(src:.c=.o)

all: $(bin) $(lib)

$(bin): $(obj)
	$(CC) $(CFLAGS) $(LDLIBS) $(LDFLAGS) $(obj) -o $@

$(lib): $(obj)
	$(CC) $(CFLAGS) $(LDLIBS) $(LDFLAGS) $(obj) -shared -o $@

miner-client: miner-client.c logger.h common.c common.h
	$(CC) $(CFLAGS) $(LDLIBS) $(LDFLAGS) miner-client.c common.c sha1.c -o $@

miner.o: miner.c miner.h sha1.h logger.h
sha1.o: sha1.c sha1.h logger.h
miner-client.o: miner-client.c miner.h sha1.h logger.h common.h task.h


clean:
	rm -f $(bin) $(lib) $(obj) vgcore.*


# Tests --

test_repo=usf-cs521-sp23/P4-Tests

test: all ./.testlib/run_tests ./tests
	@DEBUG="$(debug)" ./.testlib/run_tests $(run)

grade: ./.testlib/grade
	./.testlib/grade $(run)

testupdate: testclean test

testclean:
	rm -rf tests .testlib

./tests:
	rm -rf ./tests
	git clone https://github.com/$(test_repo) tests

./.testlib/run_tests:
	rm -rf ./.testlib
	git clone https://github.com/malensek/cowtest.git .testlib

./.testlib/grade: ./.testlib/run_tests
