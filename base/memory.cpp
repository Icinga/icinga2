#include "i2-base.h"

using namespace icinga;

void *Memory::Allocate(size_t size)
{
	void *ptr = malloc(size);

	if (size != 0 && ptr == NULL)
		throw OutOfMemoryException();

	return ptr;
}

void *Memory::Reallocate(void *ptr, size_t size)
{
	void *new_ptr = realloc(ptr, size);

	if (size != 0 && new_ptr == NULL)
		throw OutOfMemoryException();
	
	return new_ptr;
}

void Memory::Free(void *ptr)
{
	if (ptr != NULL)
		free(ptr);
}
