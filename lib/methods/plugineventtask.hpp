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

#ifndef PLUGINEVENTTASK_H
#define PLUGINEVENTTASK_H

#include "methods/i2-methods.hpp"
#include "icinga/service.hpp"
#include "base/process.hpp"

namespace icinga
{

/**
 * Implements event handlers based on external plugins.
 *
 * @ingroup methods
 */
class I2_METHODS_API PluginEventTask
{
public:
	static void ScriptFunc(const Checkable::Ptr& service,
	    const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros);

private:
	PluginEventTask(void);

	static void ProcessFinishedHandler(const Checkable::Ptr& checkable,
	    const Value& commandLine, const ProcessResult& pr);
};

}

#endif /* PLUGINEVENTTASK_H */
