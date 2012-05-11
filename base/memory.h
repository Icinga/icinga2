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

#ifndef MEMORY_H
#define MEMORY_H

namespace icinga
{

DEFINE_EXCEPTION_CLASS(OutOfMemoryException);

/**
 * Memory
 *
 * Singleton class which implements memory allocation helpers.
 */
class I2_BASE_API Memory
{
private:
	Memory(void);

public:
	static void *Allocate(size_t size);
	static void *Reallocate(void *ptr, size_t size);
	static char *StrDup(const char *str);
	static void Free(void *ptr);
};

}

#endif /* MEMORY_H */
