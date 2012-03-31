#ifndef I2_WIN32_H
#define I2_WIN32_H

#define NOGDI
#include <windows.h>
#include <imagehlp.h>

typedef int socklen_t;

#define I2_EXPORT __declspec(dllexport)
#define I2_IMPORT __declspec(dllimport)

#endif /* I2_WIN32_H */