#ifndef RENDERER_H_INCLUDED
#define RENDERER_H_INCLUDED

#include <stdint.h>
#include <xosd.h>

typedef struct renderer_api {
	int (*initialize)(void**, const void* arguments, uint64_t argumentsSize);
	void (*destroy)(void**);
	int (*tick)(void*);
	int (*show)(void*, xosd** osd, const char* arguments);
	int (*hide)(void*);
} renderer_api;

typedef struct renderer {
	const renderer_api* api;
	void *opaque;
} renderer;

int renderer_initialize(renderer**, const renderer_api* api, const void* arguments, uint64_t argumentsSize);
void renderer_destroy(renderer**);
int renderer_tick(renderer*);
int renderer_show(renderer*, xosd** osd, const char* arguments);
int renderer_hide(renderer*);

#endif
