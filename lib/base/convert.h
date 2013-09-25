/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#ifndef CONVERT_H
#define CONVERT_H

#include "base/i2-base.h"
#include "base/value.h"

namespace icinga
{

/**
 * Utility class for converting types.
 *
 * @ingroup base
 */
class I2_BASE_API Convert
{
public:
	static long ToLong(const String& val);
	static double ToDouble(const String& val);
	static bool ToBool(const String& val);
	static String ToString(const Value& val);

private:
	Convert(void);
};

}

#endif /* CONVERT_H */
