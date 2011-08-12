#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xosd.h>
#include <sys/sysinfo.h>
#include "xosdutil.h"
#include "uptime_renderer.h"
#include "log.h"

typedef struct uptime_renderer_data {
	xosd* osd;
	struct sysinfo si;
} uptime_renderer_data;

static int tick(void*);
static int hide(void*);

static int initialize(void** r, const void* arguments, uint64_t argumentsSize) {
	int f = 0;
	uptime_renderer_data* data;

	data = malloc(sizeof(uptime_renderer_data));
	if (data) {
		memset(data, 0, sizeof(uptime_renderer_data));
		if (create_xosd(&data->osd, 2) == 0) {
			*r = data;
		} else {
			msg("Failed to xosd_create.\n");
			f = 2;
		}
	} else {
		msg("Failed to allocate uptime renderer data.\n");
		f = 1;
	}
	return f;
}

static void destroy(void** r) {
	uptime_renderer_data* _r = *r;
	if (_r) {
		if (_r->osd) {
			xosd_destroy(_r->osd);
			_r->osd = NULL;
		}
	}
}

static int tick(void* r) {
	int f = 0;
	uptime_renderer_data* _r = r;
	const int len = 100;
	char buffer[len];
	int pos = 0, upsecs, upminutes, uphours, updays;

	buffer[0] = '\0';

	if (_r && _r->osd) {
		sysinfo(&_r->si);
		upsecs = _r->si.uptime % 60;
		updays = (int) _r->si.uptime / (60*60*24);
		strcat(buffer, "up ");
		pos += 3;
		if (updays) {
			pos += snprintf(buffer + pos, len - pos, "%d d, ", updays);
		}
		upminutes = (int) _r->si.uptime / 60;
		uphours = (upminutes / 60) % 24;
		upminutes %= 60;
		if (uphours) {
			pos += snprintf(buffer + pos, len - pos, "%d:%02d:%02d", uphours, upminutes, upsecs);
		} else {
			pos += snprintf(buffer + pos, len - pos, "%d:%02d", upminutes, upsecs);
		}
		if (xosd_display(_r->osd, 0, XOSD_string, buffer) < pos) {
			msg("xosd_display failed: %s\n", xosd_error);
			f = 1;
		}
		
		pos = snprintf(buffer, len, "load avg: %.2f %.2f %.2f", (double)(_r->si.loads[0])/65536.0, (double)(_r->si.loads[1])/65536.0, (double)(_r->si.loads[2])/65536.0);
		if (xosd_display(_r->osd, 1, XOSD_string, buffer) < pos) {
			msg("xosd_display failed: %s\n", xosd_error);
			f = 1;
		}
	} else {
		msg("Uptime tick failed, as the OSD window or the renderer is not initialized.\n");
		f = 1;
	}
	return f;
}

static int show(void* r, xosd** osd) {
	int f = 0;
	uptime_renderer_data* _r = r;

	if (_r && _r->osd) {
		if (xosd_show(_r->osd) == 0) {
			*osd = _r->osd;
		} else {
			msg("xosd_show failed: %s\n", xosd_error);
			f = 1;
		}
	} else {
		msg("Error: showing an uninitialized uptime renderer.\n");
		f = 1;
	}
	return f;
}

static int hide(void* r) {
	int f = 0;
	uptime_renderer_data* _r = r;

	if (_r && _r->osd) {
		if (xosd_hide(_r->osd) != 0) {
			msg("xosd_hide failed: %s\n", xosd_error);
			f = 1;
		}
	} else {
		msg("Error: hiding an uninitialized uptime renderer.\n");
		f = 1;
	}
	return f;
}

const renderer_api uptime_renderer = {
	.initialize = initialize,
	.destroy = destroy,
	.tick = tick,
	.show = show,
	.hide = hide
};
