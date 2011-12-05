#include <stdlib.h>
#include <string.h>
#include <xosd.h>
#include <time.h>
#include <libconfig.h>
#include "xosdutil.h"
#include "renderers/time.h"
#include "log.h"

typedef struct time_renderer_data {
	xosd* osd;
	char* time_format;
} time_renderer_data;

static int tick(void*);
static int hide(void*);

/**
 * Arguments: NULL or config_setting_t*
 */
static int initialize(void** r, const void* arguments, uint64_t argumentsSize) {
	int f = 0;
	time_renderer_data* data;

	data = malloc(sizeof(time_renderer_data));
	if (!data) {
		msg("Failed to allocate time renderer data.\n");
		goto err_1;
	}
	memset(data, 0, sizeof(time_renderer_data));
	if (create_xosd(&data->osd, 1) != 0) {
		msg("Failed to xosd_create.\n");
		goto err_2;
	}
	if (argumentsSize) {
		if (argumentsSize != sizeof(config_setting_t*)) {
			msg("Unexpected arguments for time renderer.\n");
			goto err_3;
		}
		const char* buffer = NULL;
		if (config_setting_lookup_string(arguments, "format", &buffer) && buffer) {
			data->time_format = strdup(buffer);
			msg("Time format wanted: %s\n", data->time_format);
		} else {
			msg("Invalid time format.\n");
		}
	}
	*r = data;

	goto out;

err_3:
	// xosd_destroy
err_2:
	free(data);
err_1:
	f = 1;
out:
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
		strftime(buffer, 100, _r->time_format ? _r->time_format : "%c", tmp);
		if (xosd_display(_r->osd, 0, XOSD_string, buffer) < pos) {
			msg("xosd_display failed: %s\n", xosd_error);
			f = 2;
		}
	} else {
		msg("Time tick failed, as the OSD window or the renderer is not initialized.\n");
		f = 1;
	}
	return f;
}

static int show(void* r, xosd** osd, const char* arguments) {
	int f = 0;
	time_renderer_data* _r = r;

	if (_r && _r->osd) {
		if (xosd_show(_r->osd) == 0) {
			*osd = _r->osd;
		} else {
			msg("xosd_show failed: %s\n", xosd_error);
			f = 2;
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
