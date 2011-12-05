//#define _XOPEN_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <string.h>
#include <libconfig.h>
#include "control_socket.h"
#include "control_pipe.h"
#include "renderers/time.h"
#include "renderers/uptime.h"
#include "renderers/battery.h"
#include "renderers/exec.h"
#include "renderers/echo.h"
#include "log.h"
#include "xosdutil.h"

#define COUNTOF(x) (sizeof(x)/sizeof(*x))

static bool run = true;
static xosd *osd;
static renderer **renderers = NULL;
static const char **renderer_commands = NULL;
static int* renderer_durations = NULL;
static int renderers_count = 0;
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
static bool use_pipe = false;

static void sigchld(int signal) {
	int status;
	wait(&status);
}

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
	if (snprintf(conf_dir, COUNTOF(conf_dir), "%s/.xosdutil", pwentry->pw_dir) > COUNTOF(conf_dir)) {
		die("Buffer overflow.\n");
	}
	if ((use_pipe && snprintf(fifo_name, COUNTOF(fifo_name), "%s/xosdutilctl", conf_dir) > COUNTOF(fifo_name)) ||
		(!use_pipe && snprintf(socket_name, COUNTOF(socket_name), "%s/xosdutil.socket", conf_dir) > COUNTOF(socket_name))) {
		die("Buffer overflow.\n");
	}

	if (stat(conf_dir, &result) < 0) {
		if (mkdir(conf_dir, 0755) < 0) {
			die("Cannot create my work directory (%s).\n", conf_dir);
		}
	} else {
		if (!S_ISDIR(result.st_mode)) {
			die("Please, delete %s and let me use it as my own directory.\n", conf_dir);
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
	} else if (strcmp(name, "echo") == 0) {
		api = &echo_renderer;
	} else {
		msg("Unknown renderer: %s. No effect.\n", name);
		return -1;
	}
	// TODO check
	return renderer_initialize(&renderers[renderers_count-1], api, settings, sizeof(settings));
}

// TODO
static void parse_configuration(config_t *config) {
	int result;
	config_lookup_bool(config, "use_pipe", &result);
	use_pipe = result;

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

static void run_renderer(renderer* r, int time, const char* arguments) {
	renderer_show(r, &osd, arguments);
	while (time--) {
		renderer_tick(r);
		sleep(1);
	}
	renderer_hide(r);
}

void parse_command(const char* command) {
	int cnt = 0;
	while (command[cnt] && command[cnt] != ' ') cnt++;
	msg("command received: [%s]\n", command);

	if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0 || strcmp(command, "end") == 0) {
		msg("quitting...\n");
		run = false;
	} else {
		for (int i = 0; i < renderers_count; i++) {
			if (strlen(renderer_commands[i]) == cnt &&
				strncmp(renderer_commands[i], command, cnt) == 0) {
				run_renderer(renderers[i], renderer_durations[i], command[cnt] ? (command + cnt) : NULL);
				break;
			}
		}
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
			die("fork() failed");
		} else if (pid > 0) {
			// Parent now forks off.
			exit(EXIT_SUCCESS);
		}
		setsid();
	}

	// Running in the child process now.
	
	load_configuration();

	struct sigaction sigchld_action;
	bzero(&sigchld_action, sizeof(struct sigaction));
	sigchld_action.sa_handler = &sigchld;
	sigaction(SIGCHLD, &sigchld_action, NULL);

	if (use_pipe) {
		open_pipe();

		while (run) {
			select_pipe();
		}

		close_pipe();
	} else {
		open_socket();

		while (run) {
			select_socket();
		}

		close_socket();
	}

	/*
	xosd_set_bar_length(osd, 50);
	xosd_display(osd, 1, XOSD_percentage, 77);
	xosd_display(osd, 2, XOSD_slider, 77);
	*/
	return f;
}
