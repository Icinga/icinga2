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

#ifndef _WIN32
#include <stdlib.h>
#endif
#include "icinga/nullchecktask.h"
#include "base/utility.h"
#include "base/convert.h"
#include "base/scriptfunction.h"
#include "base/logger_fwd.h"
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;

REGISTER_SCRIPTFUNCTION(NullCheck, &NullCheckTask::ScriptFunc);

Dictionary::Ptr NullCheckTask::ScriptFunc(const Service::Ptr&)
{
	char name[255];

	if (gethostname(name, sizeof(name)) < 0)
		strcpy(name, "<unknown host>");

	String output = "Hello from ";
	output += name;
	String perfdata = "time=" + Convert::ToString(static_cast<double>(Utility::GetTime()));

	Dictionary::Ptr cr = boost::make_shared<Dictionary>();
	cr->Set("output", output);
	cr->Set("performance_data_raw", perfdata);
	cr->Set("state", static_cast<ServiceState>(Utility::Random() % 4));

	return cr;
}

