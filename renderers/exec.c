#include <stdlib.h>
#include <string.h>
#include <xosd.h>
#include <time.h>
#include <libconfig.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "xosdutil.h"
#include "renderers/exec.h"
#include "log.h"

typedef struct exec_renderer_data {
	xosd* osd;
	const char* command;
} exec_renderer_data;

static int tick(void*);
static int hide(void*);

/**
 * Arguments: config_setting_t*
 */
static int initialize(void** r, const void* arguments, uint64_t argumentsSize) {
	int f = 0;
	exec_renderer_data* data;

	data = malloc(sizeof(exec_renderer_data));
	if (!data) {
		msg("Failed to allocate exec renderer data.\n");
		goto err_1;
	}
	memset(data, 0, sizeof(exec_renderer_data));
	if (create_xosd(&data->osd, 1) != 0) {
		msg("Failed to xosd_create.\n");
		goto err_2;
	}
	if (argumentsSize != sizeof(config_setting_t*)) {
		msg("Unexpected arguments for exec renderer.\n");
		goto err_3;
	}
	const char* buffer = NULL;
	if (config_setting_lookup_string(arguments, "run", &buffer) && buffer) {
		data->command = strdup(buffer);
		msg("Command to run: %s\n", data->command);
	} else {
		msg("Failed to initialize exec renderer: failed to read the command.\n");
		goto err_3;
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
	exec_renderer_data* _r = *r;
	if (_r) {
		if (_r->osd) {
			xosd_destroy(_r->osd);
			_r->osd = NULL;
		}
	}
}

static int tick(void* r) {
	int f = 0;
	exec_renderer_data* _r = r;
	int buffer_length = 0, buffer_capacity = 1024;
	char *buffer = NULL;
	pid_t pid;

	if (_r && _r->osd) {
		int pipefd[2];
		if (pipe(pipefd) != 0) {
			die("pipe() failed\n");
		}
		struct sigaction old_action, new_action;
		new_action.sa_handler = SIG_IGN;
		// TODO: vzniknou zombie!
		sigemptyset(&new_action.sa_mask);
		new_action.sa_flags = 0;
		sigaction(SIGCHLD, &new_action, &old_action);
		pid = fork();
		if (pid < 0) {
			die("fork() failed\n");
		} else if (pid > 0) {
			close(pipefd[1]);
			int r = 0;
			buffer = realloc(buffer, buffer_capacity);
			while ((r = read(pipefd[0], buffer + buffer_length, buffer_capacity - buffer_length)) > 0) {
				buffer_length += r;
				if (buffer_capacity - buffer_length < 1024) {
					buffer_capacity *= 2;
					buffer = realloc(buffer, buffer_capacity);
				}
			}
			close(pipefd[0]);
			msg("over.\n");
			buffer[buffer_length] = '\0';
		} else {
			close(pipefd[0]);
			int old_stdout = dup(1);
			dup2(pipefd[1], 1);
			close(pipe_fd);
			int result = system(_r->command);
			if (WEXITSTATUS(result) != 0) { // TODO: somehow buggy
				dup2(old_stdout, 1);
				msg("Warning: command %s returned %d\n", _r->command, result);
			}
			close(pipefd[1]);
			exit(EXIT_SUCCESS);
		}
		sigaction(SIGCHLD, &old_action, NULL);

		if (xosd_display(_r->osd, 0, XOSD_string, buffer) < buffer_length) {
			msg("xosd_display failed: %s\n", xosd_error);
			f = 2;
		}

		free(buffer);
	} else {
		msg("Time tick failed, as the OSD window or the renderer is not initialized.\n");
		f = 1;
	}
	return f;
}

static int show(void* r, xosd** osd, const char* arguments) {
	(void) arguments;
	int f = 0;
	exec_renderer_data* _r = r;

	if (_r && _r->osd) {
		if (xosd_show(_r->osd) == 0) {
			*osd = _r->osd;
		} else {
			msg("xosd_show failed: %s\n", xosd_error);
			f = 2;
		}
	} else {
		msg("Error: showing an uninitialized exec renderer.\n");
		f = 1;
	}
	return f;
}

static int hide(void* r) {
	int f = 0;
	exec_renderer_data* _r = r;

	if (_r && _r->osd) {
		if (xosd_hide(_r->osd) != 0) {
			msg("xosd_hide failed: %s\n", xosd_error);
			f = 1;
		}
	} else {
		msg("Error: hiding an uninitialized exec renderer.\n");
		f = 1;
	}
	return f;
}

const renderer_api exec_renderer = {
	.initialize = initialize,
	.destroy = destroy,
	.tick = tick,
	.show = show,
	.hide = hide
};
