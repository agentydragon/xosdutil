// TODO: switchable debug mode / daemon mode

//#define _XOPEN_SOURCE
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
#include <string.h>
#include <libconfig.h>
#include "renderers/time.h"
#include "renderers/uptime.h"
#include "renderers/battery.h"
#include "renderers/exec.h"
#include "log.h"
#include "xosdutil.h"

static bool run = true;
int pipe_fd;
static xosd *osd;
static renderer **renderers = NULL;
static const char **renderer_commands = NULL;
static int* renderer_durations = NULL;
static int renderers_count = 0;
static char fifo_name[100];
static char conf_file[100];
char conf_dir[100];
bool debug = true;
static const char* font = "-b&h-luxi mono-bold-r-*-*-80-*-*-*-*-*-*";
static const char* color = "yellow";
static int shadow_offset = 2;
static int outline_offset = 2;
static const char* outline_color = "black";
static int vertical_offset = 48;
static bool daemonize = false;

int create_xosd(xosd** osd, int size) {
	xosd* _osd;
	int f = 0;

	if ((_osd = xosd_create(size)) != NULL) {
		xosd_set_font(_osd, font);
		xosd_set_shadow_offset(_osd, shadow_offset);
		xosd_set_colour(_osd, color);
		xosd_set_outline_offset(_osd, outline_offset);
		xosd_set_outline_colour(_osd, outline_color);
		xosd_set_pos(_osd, XOSD_bottom);
		xosd_set_vertical_offset(_osd, vertical_offset);
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
	renderer_commands = malloc(sizeof(const char*)*3);
	renderer_commands[0] = "time";
	renderer_commands[1] = "uptime";
	renderer_commands[2] = "battery";
	renderer_durations = malloc(sizeof(int)*3);
	renderer_durations[0] = 5;
	renderer_durations[1] = 5;
	renderer_durations[2] = 5;
	renderers = malloc(sizeof(renderer*)*3);
	renderer_initialize(&renderers[0], &time_renderer, NULL, 0);
	renderer_initialize(&renderers[1], &uptime_renderer, NULL, 0);
	renderer_initialize(&renderers[2], &battery_renderer, NULL, 0);
}

static int parse_command_setting(config_setting_t *settings) {
	const renderer_api* api;
	const char* name, *command;

	if (!config_setting_lookup_string(settings, "renderer", &name)) {
		msg("no renderer specified, will have no effect.\n");
		return -1;
	}
	
	if (!config_setting_lookup_string(settings, "command", &command)) {
		msg("no command specified, will have no effect.\n");
		return -1;
	}

	renderers_count++;
	renderers = realloc(renderers, sizeof(renderer*)*renderers_count);
	renderer_commands = realloc(renderer_commands, sizeof(const char*)*renderers_count);
	renderer_durations = realloc(renderer_durations, sizeof(int)*renderers_count);

	renderer_commands[renderers_count-1] = strdup(command);
	renderer_durations[renderers_count-1] = 5;
	config_setting_lookup_int(settings, "duration", &renderer_durations[renderers_count-1]);

	if (strcmp(name, "time") == 0) {
		api = &time_renderer;
	} else if (strcmp(name, "uptime") == 0) {
		api = &uptime_renderer;
	} else if (strcmp(name, "battery") == 0) {
		api = &battery_renderer;
	} else if (strcmp(name, "exec") == 0) {
		api = &exec_renderer;
	} else {
		msg("Unknown renderer: %s. No effect.\n", name);
		return -1;
	}
	// TODO check
	return renderer_initialize(&renderers[renderers_count-1], api, settings, sizeof(settings));
}

// TODO
static void parse_configuration(config_t *config) {
	config_setting_t* section = config_lookup(config, "display");
	const char* buffer;
	if (section) {
		if (config_setting_lookup_string(section, "font", &buffer)) {
			font = strdup(buffer);
		}
		if (config_setting_lookup_string(section, "color", &buffer)) {
			color = strdup(buffer);
		}
		config_setting_lookup_int(section, "shadow_offset", &shadow_offset);
		config_setting_lookup_int(section, "outline_offset", &outline_offset);
		if (config_setting_lookup_string(section, "outline_color", &buffer)) {
			outline_color = strdup(buffer);
		}
		// TODO: position
		config_setting_lookup_int(section, "vertical_offset", &vertical_offset);
		// TODO: align
	}

	section = config_lookup(config, "commands");
	if (section) {
		config_setting_t* command;
		for (int index = 0; index < config_setting_length(section); index++) {
			command = config_setting_get_elem(section, index);
			parse_command_setting(command);
		}
	} else {
		msg("missing command section in configuration file, nothing will work!");
	}
}

static void load_configuration() {
	struct stat result;
	config_t config;
	snprintf(conf_file, 100, "%s/xosdutil.cfg", conf_dir);
	if (stat(conf_file, &result) < 0 || !S_ISREG(result.st_mode) || access(conf_file, R_OK) != 0) {
		msg("Cannot open configuration file at %s, continuing with default settings...\n", conf_file);
		load_default_configuration();
	} else {
		config_init(&config);
		if (config_read_file(&config, conf_file) != CONFIG_TRUE) {
			msg("Cannot parse configuration file (error from libconfig: %s), continuing with default settings...\n", config_error_text(&config));
			msg("Error on line %d.\n", config_error_line(&config));
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
	} else {
		for (int i = 0; i < renderers_count; i++) {
			if (strcmp(renderer_commands[i], command) == 0) {
				run_renderer(renderers[i], renderer_durations[i]);
			}
		}
	}
}

static void select_pipe() {
	static size_t length = 0, capacity = 0;
	static char* buffer = NULL;
	size_t nbytes = 0;
	fd_set readfds, writefds, exceptfds;
	struct timeval timeout;
	
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	FD_SET(pipe_fd, &readfds); // We want to check if the control pipe is ready for reading.

	timeout.tv_sec = 60;
	timeout.tv_usec = 0; // 60 seconds

	msg("select\n");
	switch (select(pipe_fd + 1, &readfds, &writefds, &exceptfds, &timeout)) {
		case 1:
			// The file descriptor is now ready to read.
			if (length >= capacity) {
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
				if (length >= capacity) {
					capacity *= 2;
					if (!capacity) capacity = 1;
					buffer = realloc(buffer, capacity);
					if (!buffer) {
						msg("Ran out of memory resizing command buffer.\n");
						exit(1);
					}
					buffer[length] = '\0';
				}
				for (int i = 0; i < length; i++) {
					if (buffer[i] == '\n') {
						buffer[i] = '\0';
						parse_command(buffer);
						memmove(buffer, buffer + i + 1, length - i);
						length -= (i + 1);
					}
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

int main(int argc, const char** argv) {
	int f = 0;
	pid_t pid;

	setlocale(LC_ALL, "");
	atexit(log_close);
	check_configuration_directory();

	if (daemonize) {
		pid = fork();
		if (pid < 0) {
			perror("fork");
			exit(EXIT_FAILURE);
		} else if (pid > 0) {
			// Parent now forks off.
			exit(EXIT_SUCCESS);
		}
		setsid();
	}

	// Running in the child process now.
	
	load_configuration();
	open_pipe();

	while (run) {
		select_pipe();
	}

	unlink(fifo_name);
	/*
	xosd_set_bar_length(osd, 50);
	xosd_display(osd, 1, XOSD_percentage, 77);
	xosd_display(osd, 2, XOSD_slider, 77);
	*/
	return f;
}
