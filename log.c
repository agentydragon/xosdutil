#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "xosdutil.h"
#include "log.h"

static FILE* log_file = NULL;

static void log_start() {
	log_file = fopen(CONF_DIR "/xosdutil.log", "w");
	if (!log_file) {
		fprintf(stderr, "Can't open log file!\n");
		exit(1);
	}
}

void log_close() {
	if (log_file) {
		fclose(log_file);
		log_file = NULL;
	}
}

void msg(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	if (!log_file) {
		log_start();
	}
	vfprintf(log_file, fmt, ap);
	va_end(ap);
}
