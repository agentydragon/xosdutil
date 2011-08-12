#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <xosd.h>
#include <time.h>
#include <sys/sysinfo.h>
#include "uptime.h"

int main(int argc, const char** argv) {
	xosd *osd;
	int f = 0;

	setlocale(LC_ALL, "");
	osd = xosd_create(2);

	if (osd) {
		//xosd_set_font(osd, "fixed");
		//xosd_set_font(osd, "-adobe-new century schoolbook-medium-r-*--60-*-*-*-*-*-*");
		//xosd_set_font(osd, "-b&h-lucida-medium-r-*-*-70-*-*-*-*-*-*");
		//xosd_set_font(osd, "-b&h-luxi mono-medium-r-*-*-60-*-*-*-*-*-*");
		xosd_set_font(osd, "-b&h-luxi mono-bold-r-*-*-75-*-*-*-*-*-*");
		//xosd_set_font(osd, "-b&h-luxi sans-medium-r-*-*-70-*-*-*-*-*-*");
		xosd_set_shadow_offset(osd, 2);
		xosd_set_colour(osd, "yellow");
		xosd_set_outline_offset(osd, 2);
		xosd_set_outline_colour(osd, "black");
		xosd_set_pos(osd, XOSD_bottom);
		xosd_set_vertical_offset(osd, 48);
		xosd_set_align(osd, XOSD_center);
		xosd_set_bar_length(osd, 50);
		/*
		xosd_display(osd, 0, XOSD_string, "Text XOSD");
		xosd_display(osd, 1, XOSD_percentage, 77);
		xosd_display(osd, 2, XOSD_slider, 77);
		*/
		/*
		sleep(1);
		xosd_scroll(osd, 1);
		xosd_display(osd, 0, XOSD_string, "Žluťoučký kůň úpěl ďábelské ódy.");
		sleep(1);
		xosd_scroll(osd, 1);
		*/
		char buffer[100];
		double load[3];
		struct sysinfo si;
		sysinfo(&si);
		struct tm *tmp;
		time_t t = time(NULL);
		tmp = localtime(&t);
		strftime(buffer, 100, "%a %d.%m.%y %H:%M:%S", tmp);
		xosd_display(osd, 0, XOSD_string, buffer);
		getloadavg(load, 3);
		snprintf(buffer, 100, "load avg: %.2f %.2f %.2f", load[0], load[1], load[2]);
		xosd_display(osd, 1, XOSD_string, buffer);
		sleep(2);
		xosd_display(osd, 0, XOSD_string, sprint_uptime());
		sleep(5);
		xosd_destroy(osd);
	} else {
		perror("Couldn't create XOSD instance.\n");
		f = 1;
	}
	return f;
}
