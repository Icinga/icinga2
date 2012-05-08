#include "i2-base.h"

using namespace icinga;

/**
 * Memory
 *
 * Constructor for the memory class.
 */
Memory::Memory(void)
{
}

/**
 * Allocate
 *
 * Allocates memory. Throws an exception if no memory is available. Alignment
 * guarantees are the same like for malloc().
 *
 * @param size The size of the requested memory block.
 * @returns A new block of memory.
 */
void *Memory::Allocate(size_t size)
{
	void *ptr = malloc(size);

	if (size != 0 && ptr == NULL)
		throw OutOfMemoryException("malloc failed.");

	return ptr;
}

/**
 * Reallocate
 *
 * Resizes a block of memory. Throws an exception if no memory is available.
 *
 * @param ptr The old memory block or NULL.
 * @param size The requested new size of the block.
 * @returns A pointer to the new memory block.
 */
void *Memory::Reallocate(void *ptr, size_t size)
{
	void *new_ptr = realloc(ptr, size);

	if (size != 0 && new_ptr == NULL)
		throw OutOfMemoryException("realloc failed.");
	
	return new_ptr;
}

/**
 * StrDup
 *
 * Duplicates a string. Throws an exception if no memory is available.
 *
 * @param str The string.
 * @returns A copy of the string.
 */
char *Memory::StrDup(const char *str)
{
	char *new_str = strdup(str);

	if (str == NULL)
		throw OutOfMemoryException("strdup failed.");

	return new_str;
}

/**
 * Free
 *
 * Frees a memory block.
 *
 * @param ptr The memory block.
 */
void Memory::Free(void *ptr)
{
	if (ptr != NULL)
		free(ptr);
}
