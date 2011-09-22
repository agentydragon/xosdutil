#ifndef XOSDUTIL_H_INCLUDED
#define XOSDUTIL_H_INCLUDED

#include <xosd.h>
#include <stdbool.h>

int create_xosd(xosd** osd, int size);

extern char conf_dir[100];
extern bool debug;
extern int pipe_fd;

#endif
