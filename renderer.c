#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "renderer.h"
#include "log.h"

int renderer_initialize(renderer** r, const renderer_api* api, const void* arguments, uint64_t argumentsSize) {
	int f = 0;
	renderer* _r = malloc(sizeof(renderer));

	if (_r) {
		memset(_r, 0, sizeof(renderer));
		_r->api = api;
		if (_r->api->initialize && _r->api->initialize(&_r->opaque, arguments, argumentsSize) == 0) {
			*r = _r;
		} else {
			msg("Renderer initialization failed.\n");
			free(_r);
			_r = NULL;
		}
	} else {
		msg("Failed to allocate memory for renderer.\n");
		*r = NULL;
		f = 1;
	}
	return f;	
}

void renderer_destroy(renderer** r) {
	renderer* _r = *r;

	if (_r && _r->api && _r->api->destroy) {
		_r->api->destroy(&_r->opaque);
		free(_r);
		_r = NULL;
		*r = _r;
	}
}

int renderer_tick(renderer* r) {
	int f = 0;
	if (!r || !r->api || !r->api->tick) {
		msg("renderer_tick called on incomplete renderer.\n");
	} else {
		f = r->api->tick(r->opaque);
	}
	return f;
}

int renderer_show(renderer* r, xosd** osd) {
	int f = 0;
	if (!r || !r->api || !r->api->show) {
		msg("renderer_show called on incomplete renderer.\n");
	} else {
		f = r->api->show(r->opaque, osd);
	}
	return f;
}

int renderer_hide(renderer* r) {
	return !r || !r->api || !r->api->show || r->api->hide(r->opaque);
}
