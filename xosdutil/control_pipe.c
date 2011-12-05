#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>

#include "xosdutil.h"
#include "log.h"
#include "control_pipe.h"

int pipe_fd;
char fifo_name[100];

void open_pipe() {
	struct stat result;
	if (stat(fifo_name, &result) < 0) {
		if (mkfifo(fifo_name, S_IWUSR | S_IRUSR) < 0) {
			die("Cannot create control pipe in %s.\n", fifo_name);
		}
	} else {
		if (!S_ISFIFO(result.st_mode)) {
			die("I have some junk in where I would expect to make my control pipe (%s). Please remove it.\n", fifo_name);
		}
	}

	pipe_fd = open(fifo_name, O_RDWR | O_NONBLOCK); // O_RDWR because we need at least one writer for select() to work.
	if (pipe_fd < 0) {
		die("Failed to open the control pipe at %s.\n", fifo_name);
	}
}

void select_pipe() {
	static size_t length = 0, capacity = 0;
	static char* buffer = NULL;
	size_t nbytes = 0;
	fd_set readfds, writefds, exceptfds;
	struct timeval timeout;
	
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	FD_SET(pipe_fd, &readfds); // We want to check if the control pipe is ready for reading.

	timeout.tv_sec = 600;
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
					die("Ran out of memory resizing command buffer.\n");
				}
			}
			nbytes = read(pipe_fd, buffer + length, capacity - length);
			if (nbytes < 0) {
				die("Pipe read() failed.\n");
			} else {
				length += nbytes;
				if (length >= capacity) {
					capacity *= 2;
					if (!capacity) capacity = 1;
					buffer = realloc(buffer, capacity);
					if (!buffer) {
						die("Ran out of memory resizing command buffer.\n");
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
			break;
		case -1:
			perror("select");
			break;
	}
}

