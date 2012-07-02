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

#include "i2-configfile.h"

using std::ifstream;

using namespace icinga;

string ConfigFileComponent::GetName(void) const
{
	return "configfilecomponent";
}

void ConfigFileComponent::Start(void)
{
	ifstream fp;
	FIFO::Ptr fifo = boost::make_shared<FIFO>();

	string filename;
	if (!GetConfig()->GetProperty("configFilename", &filename))
		throw logic_error("Missing 'configFilename' property");

	vector<ConfigItem::Ptr> configItems = ConfigCompiler::CompileFile(filename);
	ConfigVM::ExecuteItems(configItems);
}

void ConfigFileComponent::Stop(void)
{
}

EXPORT_COMPONENT(configfile, ConfigFileComponent);
