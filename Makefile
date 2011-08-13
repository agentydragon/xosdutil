CC = gcc
CFLAGS = `xosd-config --cflags --libs` -Wall -pedantic -g -std=c99 -I.
COMPILE = $(CC) $(CFLAGS) -c
OBJFILES := $(patsubst %.c,obj/%.o,$(wildcard *.c renderers/*.c))

bin/xosdutil: $(OBJFILES)
	$(CC) $(CFLAGS) -o $@ $(OBJFILES)

obj/%.o: %.c
	$(COMPILE) -o $@ $<

clean:
	@rm bin/xosdutil $(OBJFILES)

all: bin/xosdutil

.PHONY: clean
