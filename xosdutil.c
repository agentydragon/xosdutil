#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include "renderer.h"
#include "uptime_renderer.h"
#include "log.h"
#include "xosdutil.h"

static bool run = true;
static time_t hide_time;
static int pipe_fd;
static xosd *osd;
static renderer *renderer_uptime, *renderer_time;

int create_xosd(xosd** osd, int size) {
	xosd* _osd;
	int f = 0;

	if ((_osd = xosd_create(size)) != NULL) {
		xosd_set_font(_osd, "-b&h-luxi mono-bold-r-*-*-80-*-*-*-*-*-*");
		xosd_set_shadow_offset(_osd, 2);
		xosd_set_colour(_osd, "yellow");
		xosd_set_outline_offset(_osd, 2);
		xosd_set_outline_colour(_osd, "black");
		xosd_set_pos(_osd, XOSD_bottom);
		xosd_set_vertical_offset(_osd, 48);
		xosd_set_align(_osd, XOSD_center);
		*osd = _osd;
	} else {
		msg("xosd_create failed\n");
		f = 1;
	}

	return f;
}

static void check_configuration_directory() {
	struct stat result;
	if (stat(CONF_DIR, &result) < 0) {
		if (mkdir(CONF_DIR, 0755) < 0) {
			fprintf(stderr,"Cannot create my work directory (%s).\n", CONF_DIR);
			perror("mkdir");
			exit(EXIT_FAILURE);
		}
	} else {
		if (!S_ISDIR(result.st_mode)) {
			fprintf(stderr,"Please, delete %s and let me use it as my own directory.\n", CONF_DIR);
			exit(EXIT_FAILURE);
		}
	}
}

static void open_pipe() {
	struct stat result;
	if (stat(FIFO_NAME, &result) < 0) {
		if (mkfifo(FIFO_NAME, S_IWUSR | S_IRUSR) < 0) {
			msg("Cannot create control pipe in %s.\n", FIFO_NAME);
			perror("mkfifo");
			exit(EXIT_FAILURE);
		}
	} else {
		// TODO: kill all own instances and take over this.
		if (!S_ISFIFO(result.st_mode)) {
			msg("I have some junk in where I would expect to make my control pipe (%s). Please remove it.\n", FIFO_NAME);
			exit(EXIT_FAILURE);
		}
	}

	pipe_fd = open(FIFO_NAME, O_RDWR | O_NONBLOCK); // O_RDWR because we need at least one writer for select() to work.
	if (pipe_fd < 0) {
		perror("open");
		msg("Failed to open the control pipe at %s.\n", FIFO_NAME);
		exit(EXIT_FAILURE);
	}
}

static void initialize_renderers() {
}

static void run_renderer(renderer* r, int time) {
	renderer_show(r, &osd);
	while (time--) {
		renderer_tick(r);
		sleep(1);
	}
	renderer_hide(r);
}

static void parse_command(const char* command) {
	msg("command received: [%s]\n", command);
	if (strcmp(command, "exit") == 0) {
		msg("quitting...\n");
		run = false;
	} else if (strcmp(command, "uptime") == 0) {
		msg("uptime...\n");
		run_renderer(renderer_uptime, 5);
	} else if (strcmp(command, "time") == 0) {
		msg("time...\n");
		run_renderer(renderer_time, 5);
	}
}

static void select_pipe() {
	static size_t length = 0, capacity = 0;
	static char* buffer = NULL, *token = NULL;
	size_t nbytes = 0;
	fd_set readfds, writefds, exceptfds;
	struct timeval timeout;
	
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	FD_SET(pipe_fd, &readfds); // We want to check if the control pipe is ready for reading.

	timeout.tv_sec = 0;
	timeout.tv_usec = 500000; // Half a second

	printf("select...\n");
	switch (select(pipe_fd + 1, &readfds, &writefds, &exceptfds, &timeout)) {
		case 1:
			// The file descriptor is now ready to read.
			if (length == capacity) {
				capacity *= 2;
				if (!capacity) capacity = 1;
				buffer = realloc(buffer, capacity);
				if (!buffer) {
					msg("Ran out of memory resizing command buffer.\n");
					exit(1);
				}
			}
			nbytes = read(pipe_fd, buffer + length, capacity - length);
			if (nbytes < 0) {
				msg("Pipe read() failed.\n");
				perror("read");
				exit(1);
			} else {
				length += nbytes;
				if ((token = strchr(buffer, '\n')) != NULL) {
					*token = '\0';
					parse_command(buffer);
					memmove(buffer, token + 1, token - buffer + 1);
					length -= (token - buffer + 1);
				}
			}
			break;
		case 0:
			// TODO: How?
			//osd_tick();
			break;
		case -1:
			perror("select");
			break;
	}
}

static void close_pipe() {
	unlink(FIFO_NAME);
}

int main(int argc, const char** argv) {
	int f = 0;

	setlocale(LC_ALL, "");
	atexit(log_close);
	atexit(close_pipe);
	check_configuration_directory();
	open_pipe();
	initialize_renderers();

	if (renderer_initialize(&uptime, &uptime_renderer, NULL, 0) == 0) {
	} else {
		msg("Uptime renderer failed to initialize.\n");
		f = 1;
	}

	while (run) {
		select_pipe();
	}

	/*
	"-adobe-new century schoolbook-medium-r-*--60-*-*-*-*-*-*", "-b&h-lucida-medium-r-*-*-70-*-*-*-*-*-*", "-b&h-luxi mono-medium-r-*-*-60-*-*-*-*-*-*", "-b&h-luxi sans-medium-r-*-*-70-*-*-*-*-*-*"
	xosd_set_bar_length(osd, 50);

	xosd_display(osd, 1, XOSD_percentage, 77);
	xosd_display(osd, 2, XOSD_slider, 77);

	sleep(1);
	xosd_scroll(osd, 1);
	xosd_display(osd, 0, XOSD_string, "Žluťoučký kůň úpěl ďábelské ódy.");
	sleep(1);
	xosd_scroll(osd, 1);

	char buffer[100];
	double load[3];
	struct sysinfo si;
	sysinfo(&si);
	struct tm *tmp;
	time_t t = time(NULL);
	tmp = localtime(&t);
	strftime(buffer, 100, "%a %d.%m.%y %H:%M:%S", tmp);
	*/
	return f;
}
