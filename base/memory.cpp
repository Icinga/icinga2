/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "i2-base.h"

using namespace icinga;

/**
 * Constructor for the memory class.
 */
Memory::Memory(void)
{
}

/**
 * Allocates memory. Alignment guarantees are the same like for malloc().
 *
 * @param size The size of the requested memory block.
 * @returns A new block of memory.
 * @exception OutOfMemoryException No more memory is available.
 */
void *Memory::Allocate(size_t size)
{
	void *ptr = malloc(size);

	if (size != 0 && ptr == NULL)
		throw OutOfMemoryException("malloc failed.");

	return ptr;
}

/**
 * Resizes a block of memory.
 *
 * @param ptr The old memory block or NULL.
 * @param size The requested new size of the block.
 * @returns A pointer to the new memory block.
 * @exception OutOfMemoryException No more memory is available.
 */
void *Memory::Reallocate(void *ptr, size_t size)
{
	void *new_ptr = realloc(ptr, size);

	if (size != 0 && new_ptr == NULL)
		throw OutOfMemoryException("realloc failed.");
	
	return new_ptr;
}

/**
 * Duplicates a string.
 *
 * @param str The string.
 * @returns A copy of the string.
 * @exception OutOfMemoryException No more memory is available.
 */
char *Memory::StrDup(const char *str)
{
	char *new_str = strdup(str);

	if (str == NULL)
		throw OutOfMemoryException("strdup failed.");

	return new_str;
}

/**
 * Frees a memory block.
 *
 * @param ptr The memory block.
 */
void Memory::Free(void *ptr)
{
	if (ptr != NULL)
		free(ptr);
}
