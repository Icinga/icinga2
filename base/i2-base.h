#ifndef I2_BASE_H
#define I2_BASE_H

#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cerrno>

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

#include "mutex.h"
#include "condvar.h"
#include "thread.h"
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
#include "configobject.h"
#include "confighive.h"
#include "application.h"
#include "component.h"

#endif /* I2_BASE_H */