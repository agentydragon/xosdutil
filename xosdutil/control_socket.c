#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "xosdutil.h"
#include "log.h"
#include "control_socket.h"

static int socket_fd;
char socket_name[100];

void open_socket() {
	struct stat result;
	if (stat(socket_name, &result) >= 0) {
		die("I have some junk in where I would expect to make my control socket (%s). Please remove it.\n", socket_name);
	}

	socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		die("socket() failed\n");
	}
	struct sockaddr_un address;
	bzero(&address, sizeof(address));
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, socket_name, sizeof(address.sun_path));

	if (bind(socket_fd, (struct sockaddr*)&address, sizeof(address)) != 0) {
		die("bind() failed\n");
	}

	if (listen(socket_fd, 5) != 0) {
		die("listen() failed\n");
	}
}

static void handle_socket_connection(int fd) {
	char* buffer = NULL;
	size_t length = 0, capacity = 0;
	int nbytes;
	do {
		if (length >= capacity) {
			capacity *= 2;
			if (!capacity) capacity = 1;
			buffer = realloc(buffer, capacity);
			if (!buffer) {
				die("Ran out of memory resizing command buffer.\n");
			}
		}
		nbytes = read(fd, buffer + length, capacity - length);
		if (nbytes > 0) {
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
		} else if (!nbytes) {
			break;
		}
	} while (true);

	free(buffer);
	close(fd);
}

void select_socket() {
	int connection;
	struct sockaddr_un address;
	socklen_t length;
	while ((connection = accept(socket_fd, (struct sockaddr*)&address, &length)) > -1) {
		if (!fork()) {
			handle_socket_connection(connection);
		}
		close(connection);
	}
	close(socket_fd);
	unlink(socket_name);
}

