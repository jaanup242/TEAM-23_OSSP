CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -Iinclude
LIB     = src/tlb.c src/page_table.c src/translation.c

.PHONY: all test run clean
all: build/memtrace build/test_engine

build/memtrace: $(LIB) src/main.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^

build/test_engine: $(LIB) tests/test_engine.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^

test: build/test_engine
	./build/test_engine

run: all
	python3 server/server.py

clean:
	rm -rf build
