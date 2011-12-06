#ifndef XOSDUTIL_H_INCLUDED
#define XOSDUTIL_H_INCLUDED

#include <xosd.h>
#include <stdbool.h>

int create_xosd(xosd** osd, int size);
void parse_command(const char* command);
void after_fork();

extern char conf_dir[100];
extern char fifo_name[100];
extern char socket_name[100];
extern bool debug;

#endif
