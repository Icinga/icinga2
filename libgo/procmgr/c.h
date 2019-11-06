#pragma once

#include <stddef.h>
// size_t

#include <stdint.h>
// uint64_t
// uintptr_t

#include <sys/types.h>
// pid_t

typedef unsigned char uchar;

static inline
void invokeCallback(
	uintptr_t cb, uintptr_t cbData, uint64_t iPid, pid_t pid, int exitCode,
	uintptr_t output, size_t outputLen, double execStart, double execEnd
)
{
	(*(void(*)(void*, uint64_t, pid_t, int, const char*, size_t, double, double))cb)(
		(void*)cbData, iPid, pid, exitCode, (const char*)output, outputLen, execStart, execEnd
	);
}
