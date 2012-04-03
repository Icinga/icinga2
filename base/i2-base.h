#ifndef I2BASE_H
#define I2BASE_H

#ifndef _MSC_VER
#	include "config.h"
#endif /* _MSC_VER */

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
#include <map>
#include <list>

#ifdef HAVE_GCC_ABI_DEMANGLE
#	include <cxxabi.h>
#endif /* HAVE_GCC_ABI_DEMANGLE */

using namespace std;

#ifdef _MSC_VER
#	include <memory>
#	include <functional>

using namespace std::placeholders;
#else
#	include <tr1/memory>
#	include <tr1/functional>

using namespace std::tr1;
using namespace std::tr1::placeholders;
#endif

#define PLATFORM_WINDOWS 1
#define PLATFORM_UNIX 2

#ifdef _WIN32
#	define I2_PLATFORM PLATFORM_WINDOWS
#	include "win32.h"
#else
#	define I2_PLATFORM PLATFORM_UNIX
#	include "unix.h"
#endif

#include "mutex.h"
#include "condvar.h"
#include "thread.h"
#include "object.h"
#include "exception.h"
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

#endif /* I2BASE_H */
