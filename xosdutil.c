#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <pwd.h>
#include <libconfig.h>
#include "renderer.h"
#include "renderers/time.h"
#include "renderers/uptime.h"
#include "renderers/battery.h"
#include "log.h"
#include "xosdutil.h"

static bool run = true;
static int pipe_fd;
static xosd *osd;
static renderer **renderers;
static char fifo_name[100];
static char conf_file[100];
char conf_dir[100];

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
	struct passwd* pwentry = getpwuid(getuid());
	snprintf(conf_dir, 100, "%s/.xosdutil", pwentry->pw_dir);
	snprintf(fifo_name, 100, "%s/xosdutilctl", conf_dir);

	if (stat(conf_dir, &result) < 0) {
		if (mkdir(conf_dir, 0755) < 0) {
			fprintf(stderr, "Cannot create my work directory (%s).\n", conf_dir);
			perror("mkdir");
			exit(EXIT_FAILURE);
		}
	} else {
		if (!S_ISDIR(result.st_mode)) {
			fprintf(stderr, "Please, delete %s and let me use it as my own directory.\n", conf_dir);
			exit(EXIT_FAILURE);
		}
	}
}

static void load_default_configuration() {
	renderers = malloc(sizeof(renderer*)*2);
	const char time_format[] = "%a %d.%m.%y %H:%M:%S";
	renderer_initialize(&renderers[0], &time_renderer, NULL, 0);
	renderer_initialize(&renderers[1], &uptime_renderer, time_format, strlen(time_format));
	renderer_initialize(&renderers[2], &battery_renderer, NULL, 0);
}

// TODO
static void parse_configuration(config_t *config) {
}

static void load_configuration() {
	struct stat result;
	config_t config;
	snprintf(conf_file, 100, "%s/xosdutil.cfg", conf_dir);
	if (stat(conf_file, &result) < 0 || !S_ISREG(result.st_mode) || access(conf_file, R_OK) != 0) {
		msg("Cannot open configuration file at %s, continuing with default settings...", conf_file);
		load_default_configuration();
	} else {
		config_init(&config);
		if (config_read_file(&config, conf_file) != CONFIG_TRUE) {
			msg("Cannot parse configuration file (error from libconfig: %s), continuing with default settings...", config_error_text(&config));
		}
		parse_configuration(&config);
		config_destroy(&config);
	}
}

static void open_pipe() {
	struct stat result;
	if (stat(fifo_name, &result) < 0) {
		if (mkfifo(fifo_name, S_IWUSR | S_IRUSR) < 0) {
			msg("Cannot create control pipe in %s.\n", fifo_name);
			perror("mkfifo");
			exit(EXIT_FAILURE);
		}
	} else {
		// TODO: kill all own instances and take over this.
		if (!S_ISFIFO(result.st_mode)) {
			msg("I have some junk in where I would expect to make my control pipe (%s). Please remove it.\n", fifo_name);
			exit(EXIT_FAILURE);
		}
	}

	pipe_fd = open(fifo_name, O_RDWR | O_NONBLOCK); // O_RDWR because we need at least one writer for select() to work.
	if (pipe_fd < 0) {
		perror("open");
		msg("Failed to open the control pipe at %s.\n", fifo_name);
		exit(EXIT_FAILURE);
	}
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
		run = false;
	} else if (strcmp(command, "uptime") == 0) {
		run_renderer(renderers[1], 5);
	} else if (strcmp(command, "time") == 0) {
		run_renderer(renderers[0], 5);
	} else if (strcmp(command, "battery") == 0) {
		run_renderer(renderers[2], 3);
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
	unlink(fifo_name);
}

int main(int argc, const char** argv) {
	int f = 0;
	pid_t pid;

	setlocale(LC_ALL, "");
	atexit(log_close);
	check_configuration_directory();

	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	} else if (pid > 0) {
		// Parent now forks off.
		exit(EXIT_SUCCESS);
	}
	setsid();

	// Running in the child process now.
	
	load_configuration();
	open_pipe();
	atexit(close_pipe);

	while (run) {
		select_pipe();
	}
	/*
	"-adobe-new century schoolbook-medium-r-*--60-*-*-*-*-*-*", "-b&h-lucida-medium-r-*-*-70-*-*-*-*-*-*", "-b&h-luxi mono-medium-r-*-*-60-*-*-*-*-*-*", "-b&h-luxi sans-medium-r-*-*-70-*-*-*-*-*-*"
	xosd_set_bar_length(osd, 50);
	xosd_display(osd, 1, XOSD_percentage, 77);
	xosd_display(osd, 2, XOSD_slider, 77);
	*/
	return f;
}
