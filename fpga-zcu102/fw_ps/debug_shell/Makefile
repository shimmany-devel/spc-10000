CC ?= gcc
CFLAGS ?= -O2 -Wall

all: debug_shell

debug_shell: main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f debug_shell

.PHONY: all clean
