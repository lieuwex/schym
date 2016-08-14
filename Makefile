CC ?= g++
CFLAGS = -Wall -Wextra -std=c11 -g -fwrapv

SRC_FILES = $(shell find src/ -name '*.h' -o -name '*.c')
BIN ?= main


.PHONY: all clean remake

all: $(BIN)

clean:
	rm -f $(BIN)

remake: clean all


$(BIN): $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)
