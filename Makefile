CC = gcc
CFLAGS = `xosd-config --cflags --libs` -Wall -pedantic -g
COMPILE = $(CC) $(CFLAGS) -c
OBJFILES := $(patsubst %.c,%.o,$(wildcard *.c))

xosdutil: xosdutil.o uptime.o
	$(CC) $(CFLAGS) -o $@ $(OBJFILES)

%.o: %.c
	$(COMPILE) -o $@ $<

clean:
	@rm xosdutil $(OBJFILES)

all: xosdutil

.PHONY: clean
