CC = gcc
CFLAGS = `xosd-config --cflags --libs` -Wall -pedantic -g -std=gnu99 -I. -lconfig
COMPILE = $(CC) $(CFLAGS) -c
OBJFILES := $(patsubst %.c,obj/%.o,$(wildcard *.c renderers/*.c))

bin/xosdutil: $(OBJFILES)
	$(CC) $(CFLAGS) -o $@ $(OBJFILES)

obj/%.o: %.c
	$(COMPILE) -o $@ $<

clean:
	@rm bin/xosdutil $(OBJFILES)

install:
	cp bin/xosdutil /usr/local/bin/xosdutil

all: bin/xosdutil

.PHONY: clean
