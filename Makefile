CC = gcc
CFLAGS = `xosd-config --cflags --libs` -Wall -pedantic -g -std=c99
COMPILE = $(CC) $(CFLAGS) -c
OBJFILES := $(patsubst %.c,%.o,$(wildcard *.c))

xosdutil: $(OBJFILES)
	$(CC) $(CFLAGS) -o $@ $(OBJFILES)

%.o: %.c
	$(COMPILE) -o $@ $<

clean:
	@rm xosdutil $(OBJFILES)

all: xosdutil

.PHONY: clean
