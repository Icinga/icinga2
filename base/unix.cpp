#include "i2-base.h"

#if I2_PLATFORM == PLATFORM_UNIX
#include <ltdl.h>

using namespace icinga;

/**
 * Sleep
 *
 * Sleeps for the specified amount of time.
 *
 * @param milliseconds The amount of time in milliseconds.
 */
void Sleep(unsigned long milliseconds)
{
	usleep(milliseconds * 1000);
}

/**
 * closesocket
 *
 * Closes a socket.
 *
 * @param fd The socket that is to be closed.
 */
void closesocket(SOCKET fd)
{
	close(fd);
}

#endif /* I2_PLATFORM == PLATFORM_UNIX */
