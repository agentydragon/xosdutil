#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include "log.h"

#define COUNTOF(x) (sizeof(x)/sizeof(*x))

void usage(const char* program) {
	printf("Usage: %s [xosdutil commands] or %s < \"[xosdutil commands]\"\n", program, program);
}

int main(int argc, char** argv) {
	int i;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0 ||
			strcmp(argv[i], "--help") == 0) {
			usage(argv[0]);
			return 0;
		}
	}

	struct sockaddr_un address;
	int socket_fd, nread;
	char buffer[256];

	char conf_dir[100];
	char socket_name[100];

	struct stat result;
	struct passwd* pwentry = getpwuid(getuid());
	if (snprintf(conf_dir, COUNTOF(conf_dir), "%s/.xosdutil", pwentry->pw_dir) > COUNTOF(conf_dir)) {
		die("Buffer overflow.\n");
	}
	if (snprintf(socket_name, COUNTOF(socket_name), "%s/xosdutil.socket", conf_dir) > COUNTOF(socket_name)) {
		die("Buffer overflow.\n");
	}
	if (stat(conf_dir, &result) < 0 || stat(socket_name, &result) < 0) {
		die("xosdutil doesn't seem to be running...\n");
	}

	socket_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		die("socket() failed\n");
	}

	bzero(&address, sizeof(struct sockaddr_un));
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, socket_name, sizeof(address.sun_path));
	
	if (connect(socket_fd, (struct sockaddr*)&address, sizeof(struct sockaddr_un)) != 0) {
		die("connect() failed\n");
	}

	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			write(socket_fd, argv[i], strlen(argv[i]));
		}
	} else {
		do {
			nread = read(1, buffer, 255);
			if (nread > 0) {
				write(socket_fd, buffer, nread);
			} else if (nread < 0) {
				die("read() failed\n");
			} else {
				break;
			}
		} while (1);
		// after everything is sent, write a dummy newline.
		write(socket_fd, "\n", 1);
	}
	close(socket_fd);
	return 0;
}
