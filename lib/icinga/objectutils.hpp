/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef OBJECTUTILS_H
#define OBJECTUTILS_H

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include "icinga/service.hpp"

namespace icinga
{

/**
 * @ingroup icinga
 */
class I2_ICINGA_API ObjectUtils
{
public:
	static Service::Ptr GetService(const String& host, const String& name);

private:
	ObjectUtils(void);
};

}

#endif /* OBJECTUTILS_H */
