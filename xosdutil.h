#ifndef XOSDUTIL_H_INCLUDED
#define XOSDUTIL_H_INCLUDED

#include <xosd.h>

#define CONF_DIR "~/.xosdutil"
#define FIFO_NAME CONF_DIR "/xosdutilctl"

int create_xosd(xosd** osd, int size);

#endif
