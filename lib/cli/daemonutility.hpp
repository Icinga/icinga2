/******************************************************************************
* Icinga 2                                                                   *
* Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#ifndef DAEMONUTILITY_H
#define DAEMONUTILITY_H

#include "cli/i2-cli.hpp"
#include "base/string.hpp"
#include <boost/program_options.hpp>

namespace icinga
{

/**
 * @ingroup cli
 */
class I2_CLI_API DaemonUtility
{
public:
	static bool ValidateConfigFiles(const std::vector<std::string>& configs, const String& objectsFile = String());
	static bool LoadConfigFiles(const std::vector<std::string>& configs, const String& appType, const String& objectsFile = String(), const String& varsfile = String());
};

}

#endif /* DAEMONULITIY_H */
