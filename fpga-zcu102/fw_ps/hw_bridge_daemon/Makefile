CC ?= gcc
CFLAGS ?= -O2 -Wall

all: hw_bridge_daemon

hw_bridge_daemon: main.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f hw_bridge_daemon

.PHONY: all clean
