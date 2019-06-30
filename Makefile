CC ?= g++
CFLAGS = -Wall -Wextra -std=c11 -O3 -fwrapv -lm

SRC_FILES = $(shell find src/ -name '*.h' -o -name '*.c') schym.c
BIN ?= main


.PHONY: all clean remake test

all: $(BIN)

clean:
	rm -f $(BIN)

remake: clean all

test: $(BIN)
	./test.sh

$(BIN): $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)
