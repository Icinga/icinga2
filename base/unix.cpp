#include "i2-base.h"

#if I2_PLATFORM == PLATFORM_UNIX
#include <ltdl.h>

using namespace icinga;

void Sleep(unsigned long milliseconds)
{
	usleep(milliseconds * 1000);
}

void closesocket(SOCKET fd)
{
	close(fd);
}

#endif /* I2_PLATFORM == PLATFORM_UNIX */
