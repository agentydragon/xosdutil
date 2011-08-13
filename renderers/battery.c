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

typedef struct battery_renderer_data {
	xosd* osd;
	int fd_status;
	int fd_charge_full;
	int fd_charge_now;
} battery_renderer_data;

// Arguments expected: battery name (e.g. "PNPOCOA:00")
static int initialize(void** r, const void* arguments, uint64_t argumentsSize) {
	int f = 0;
	battery_renderer_data* data;

	if (!argumentsSize) {
		msg("Wrong arguments for battery_renderer initialize: expecting a battery name (e.g. \"PNPOCOA:00\")\n");
		goto err_1;
	}

	data = malloc(sizeof(battery_renderer_data));
	if (!data) {
		msg("Failed to allocate battery renderer data.\n");
		goto err_2;
	}
	
	memset(data, 0, sizeof(battery_renderer_data));
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
	battery_renderer_data* _r = *r;
	if (_r) {
		if ((_r->fd_status && close(_r->fd_status) < 0) ||
			(_r->fd_charge_full && close(_r->fd_charge_full) < 0) ||
			(_r->fd_charge_now && close(_r->fd_charge_now) < 0)) {
			perror("close");
			msg("Closing the files in battery renderer failed.");
		}
		_r->fd_status = _r->fd_charge_full = _r->fd_charge_now = 0;
		if (_r->osd) {
			xosd_destroy(_r->osd);
			_r->osd = NULL;
		}
	}
}

static int tick(void* r) {
	battery_renderer_data* _r = r;
	int f = 0;
	return f;
}

static int show(void* r, xosd** osd) {
	int f = 0;
	battery_renderer_data* _r = r;
	if (_r && _r->osd) {
		if (xosd_show(_r->osd) == 0) {
			*osd = _r->osd;
		} else {
			msg("xosd_show failed: %s\n", xosd_error);
			f = 2;
		}
	} else {
		msg("Error: showing an uninitialized battery renderer.\n");
		f = 1;
	}
	return f;
}

static int hide(void* r) {
	int f = 0;
	battery_renderer_data* _r = r;

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

const renderer_api battery_renderer = {
	.initialize = initialize,
	.destroy = destroy,
	.tick = tick,
	.show = show,
	.hide = hide
};
