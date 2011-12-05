#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "log.h"

void msg(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	va_start(ap, fmt);
	va_end(ap);
}

void die(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	va_start(ap, fmt);
	exit(EXIT_FAILURE);
}
