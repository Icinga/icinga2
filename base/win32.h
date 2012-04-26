#ifndef WIN32_H
#define WIN32_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <imagehlp.h>
#include <shlwapi.h>

typedef int socklen_t;

#define MAXPATHLEN MAX_PATH

#define I2_EXPORT __declspec(dllexport)
#define I2_IMPORT __declspec(dllimport)

#endif /* WIN32_H */
