#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include "uptime.h"

static char buf[200];

char* sprint_uptime() {
 	int upminutes, uphours, updays;
 	int pos = 0;
	struct sysinfo si;
	long uptime_secs;

	sysinfo(&si);
	uptime_secs = si.uptime;
	
 	updays = (int) uptime_secs / (60*60*24);
	strcat (buf, "up ");
	pos += 3;
	if (updays)	pos += sprintf(buf + pos, "%d day%s, ", updays, (updays != 1) ? "s" : "");
 	upminutes = (int) uptime_secs / 60;
	uphours = upminutes / 60;
 	uphours = uphours % 24;
 	upminutes = upminutes % 60;
 	if (uphours) {
		pos += sprintf(buf + pos, "%2d:%02d", uphours, upminutes);
	} else {
		pos += sprintf(buf + pos, "%d min", upminutes);
	}
	
	return buf;
} 
