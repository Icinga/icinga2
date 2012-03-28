#ifndef I2_BASE_H
#define I2_BASE_H

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <list>
#include <typeinfo>

#include <list>
#ifdef _MSC_VER
#	include <memory>
#	include <functional>
#else
#	include <tr1/memory>
#	include <tr1/functional>
#endif

#define PLATFORM_WINDOWS 1
#define PLATFORM_UNIX 2

#ifdef _WIN32
#	define I2_PLATFORM Platform_Windows
#	include "win32.h"
#else
#	define I2_PLATFORM Platform_Unix
#	include "unix.h"
#endif

#include "object.h"
#include "memory.h"
#include "delegate.h"
#include "event.h"
#include "timer.h"
#include "fifo.h"
#include "socket.h"
#include "tcpsocket.h"
#include "tcpclient.h"
#include "tcpserver.h"
#include "application.h"

#endif /* I2_BASE_H */