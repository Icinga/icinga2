/* Icinga 2 | (c) 2023 Icinga GmbH | GPLv2+ */

#ifndef _WIN32

#include <cstddef>

extern "C" void* malloc(size_t)
{
	return nullptr;
}

extern "C" void free(void*)
{
}

#endif /* _WIN32 */
