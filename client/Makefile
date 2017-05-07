CC=gcc
CFLAGS=-Wall -std=gnu99
LDLIBS= -lpthread

SOURCES=$(wildcard *.c)
HEADERS=$(wildcard *.h)

.PHONY: clean

all: client

clean:
	rm client

client: $(SOURCES) $(HEADERS)
	$(CC) $(SOURCES) -o client $(CFLAGS) $(LDLIBS)
