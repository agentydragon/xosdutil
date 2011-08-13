#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xosd.h>
#include <time.h>
#include "xosdutil.h"
#include "renderers/time.h"
#include "log.h"

typedef struct time_renderer_data {
	xosd* osd;
} time_renderer_data;

static int tick(void*);
static int hide(void*);

static int initialize(void** r, const void* arguments, uint64_t argumentsSize) {
	int f = 0;
	time_renderer_data* data;

	data = malloc(sizeof(time_renderer_data));
	if (data) {
		memset(data, 0, sizeof(time_renderer_data));
		if (create_xosd(&data->osd, 1) == 0) {
			*r = data;
		} else {
			msg("Failed to xosd_create.\n");
			f = 2;
		}
	} else {
		msg("Failed to allocate time renderer data.\n");
		f = 1;
	}
	return f;
}

static void destroy(void** r) {
	time_renderer_data* _r = *r;
	if (_r) {
		if (_r->osd) {
			xosd_destroy(_r->osd);
			_r->osd = NULL;
		}
	}
}

static int tick(void* r) {
	int f = 0;
	time_renderer_data* _r = r;
	const int len = 100;
	char buffer[len];
	struct tm *tmp;
	int pos = 0;
	time_t t;

	buffer[0] = '\0';

	if (_r && _r->osd) {
		t = time(NULL);
		tmp = localtime(&t);
		strftime(buffer, 100, "%a %d.%m.%y %H:%M:%S", tmp);
		if (xosd_display(_r->osd, 0, XOSD_string, buffer) < pos) {
			msg("xosd_display failed: %s\n", xosd_error);
			f = 1;
		}
	} else {
		msg("Time tick failed, as the OSD window or the renderer is not initialized.\n");
		f = 1;
	}
	return f;
}

static int show(void* r, xosd** osd) {
	int f = 0;
	time_renderer_data* _r = r;

	if (_r && _r->osd) {
		if (xosd_show(_r->osd) == 0) {
			*osd = _r->osd;
		} else {
			msg("xosd_show failed: %s\n", xosd_error);
			f = 1;
		}
	} else {
		msg("Error: showing an uninitialized time renderer.\n");
		f = 1;
	}
	return f;
}

static int hide(void* r) {
	int f = 0;
	time_renderer_data* _r = r;

	if (_r && _r->osd) {
		if (xosd_hide(_r->osd) != 0) {
			msg("xosd_hide failed: %s\n", xosd_error);
			f = 1;
		}
	} else {
		msg("Error: hiding an uninitialized time renderer.\n");
		f = 1;
	}
	return f;
}

const renderer_api time_renderer = {
	.initialize = initialize,
	.destroy = destroy,
	.tick = tick,
	.show = show,
	.hide = hide
};