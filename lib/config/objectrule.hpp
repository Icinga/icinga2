/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#ifndef OBJECTRULE_H
#define OBJECTRULE_H

#include "config/i2-config.hpp"
#include "config/expression.hpp"
#include "base/debuginfo.hpp"
#include <set>

namespace icinga
{

/**
 * @ingroup config
 */
class I2_CONFIG_API ObjectRule
{
public:
	typedef std::set<String> TypeSet;

	static void RegisterType(const String& sourceType);
	static bool IsValidSourceType(const String& sourceType);

private:
	ObjectRule(void);

	static TypeSet m_Types;
};

}

#endif /* OBJECTRULE_H */
