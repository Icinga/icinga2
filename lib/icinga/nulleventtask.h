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

#ifndef NULLEVENTTASK_H
#define NULLEVENTTASK_H

#include "icinga/i2-icinga.h"
#include "icinga/service.h"
#include "base/dictionary.h"

namespace icinga
{

/**
 * Test class for additional event handler types. Implements the "null" event handler type.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API NullEventTask
{
public:
	static void ScriptFunc(const Service::Ptr& service);

private:
	NullEventTask(void);
};

}

#endif /* NULLEVENTTASK_H */
