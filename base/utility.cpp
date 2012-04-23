#include "i2-base.h"

using namespace icinga;

/**
 * Daemonize
 *
 * Detaches from the controlling terminal.
 */
void Utility::Daemonize(void) {
#ifndef _WIN32
	pid_t pid;
	int fd;

	pid = fork();
	if (pid < 0)
		throw PosixException("fork failed", errno);

	if (pid)
		exit(0);

	fd = open("/dev/null", O_RDWR);

	if (fd < 0)
		throw PosixException("open failed", errno);

	if (fd != 0)
		dup2(fd, 0);

	if (fd != 1)
		dup2(fd, 1);

	if (fd != 2)
		dup2(fd, 2);

	if (fd > 2)
		close(fd);

	if (setsid() < 0)
		throw PosixException("setsid failed", errno);
#endif
}
