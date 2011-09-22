#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xosd.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include "xosdutil.h"
#include "renderers/time.h"
#include "log.h"

typedef struct echo_renderer_data {
	xosd* osd;
	char* message;
} echo_renderer_data;

static int initialize(void** r, const void* arguments, uint64_t argumentsSize) {
	int f = 0;
	echo_renderer_data* data;

	if (!argumentsSize) {
		msg("Wrong arguments for echo_renderer initialize: expecting a echo name (e.g. \"PNPOCOA:00\")\n");
		goto err_1;
	}

	data = malloc(sizeof(echo_renderer_data));
	if (!data) {
		msg("Failed to allocate echo renderer data.\n");
		goto err_2;
	}
	
	memset(data, 0, sizeof(echo_renderer_data));
	if (create_xosd(&data->osd, 1) != 0) {
		msg("Failed to xosd_create.\n");
		goto err_3;
	}
	*r = data;
	goto out;

err_3:
	;	
err_2:
	free(data);	
err_1:
	f = 1;
out:
	return f;
}

static void destroy(void** r) {
	echo_renderer_data* _r = *r;
	if (_r) {
		if (_r->message) {
			free(_r->message);
			_r->message = NULL;
		}
		if (_r->osd) {
			xosd_destroy(_r->osd);
			_r->osd = NULL;
		}
	}
}

static int tick(void* r) {
	echo_renderer_data* _r = r;
	int f = 0;

	if (_r && _r->osd) {
		if (xosd_display(_r->osd, 0, XOSD_string, _r->message) < strlen(_r->message)) {
			msg("xosd_display failed: %s\n", xosd_error);
			f = 1;
		}
	} else {
		msg("Echo tick failed, as the OSD window or the renderer is not initialized.\n");
		f = 1;
	}

	return f;
}

// TODO: parse length, color, ...
static int show(void* r, xosd** osd, const char* arguments) {
	int f = 0;
	echo_renderer_data* _r = r;

	if (_r && _r->osd) {
		if (_r->message) {
			free(_r->message);
			_r->message = NULL;
		}
		_r->message = strdup(arguments);
		if (xosd_show(_r->osd) == 0) {
			*osd = _r->osd;
		} else {
			msg("xosd_show failed: %s\n", xosd_error);
			f = 2;
		}
	} else {
		msg("Error: showing an uninitialized echo renderer.\n");
		f = 1;
	}
	return f;
}

static int hide(void* r) {
	int f = 0;
	echo_renderer_data* _r = r;

	if (_r && _r->osd) {
		if (xosd_hide(_r->osd) != 0) {
			msg("xosd_hide failed: %s\n", xosd_error);
			f = 2;
		}
	} else {
		msg("Error: hiding an uninitialized time renderer.\n");
		f = 1;
	}
	return f;
}

const renderer_api echo_renderer = {
	.initialize = initialize,
	.destroy = destroy,
	.tick = tick,
	.show = show,
	.hide = hide
};
