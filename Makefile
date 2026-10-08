CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2

.PHONY: all test clean

all: micropascal

micropascal: src/micropascal.c
	$(CC) $(CFLAGS) $< -o $@

test: micropascal
	python3 tests/test.py

clean:
	rm -f micropascal
