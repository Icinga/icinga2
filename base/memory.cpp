#include "i2-base.h"

using namespace icinga;

Memory::Memory(void)
{
}

void *Memory::Allocate(size_t size)
{
	void *ptr = malloc(size);

	if (size != 0 && ptr == NULL)
		throw OutOfMemoryException("malloc failed.");

	return ptr;
}

void *Memory::Reallocate(void *ptr, size_t size)
{
	void *new_ptr = realloc(ptr, size);

	if (size != 0 && new_ptr == NULL)
		throw OutOfMemoryException("realloc failed.");
	
	return new_ptr;
}

char *Memory::StrDup(const char *str)
{
	char *new_str = strdup(str);

	if (str == NULL)
		throw OutOfMemoryException("strdup failed.");

	return new_str;
}

void Memory::Free(void *ptr)
{
	if (ptr != NULL)
		free(ptr);
}
