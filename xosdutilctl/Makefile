CC = gcc
CFLAGS = -Wall -pedantic -g -std=gnu99 -I. -lconfig
COMPILE = $(CC) $(CFLAGS) -c
OBJFILES := $(patsubst %.c,obj/%.o,$(wildcard *.c renderers/*.c))

bin/xosdutilctl: $(OBJFILES)
	$(CC) $(CFLAGS) -o $@ $(OBJFILES)

obj/%.o: %.c
	$(COMPILE) -o $@ $<

clean:
	@rm bin/xosdutilctl $(OBJFILES)

install:
	cp bin/xosdutilctl /usr/local/bin/xosdutilctl

all: bin/xosdutilctl

.PHONY: clean
