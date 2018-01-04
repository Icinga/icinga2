/******************************************************************************
* Icinga 2                                                                   *
* Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "icinga/icingaapplication.hpp"
#include "base/application.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

static bool init_unit_test()
{
	return true;
}

int main(int argc, char *argv[])
{
	Application::InitializeBase();

	IcingaApplication::Ptr appInst;

	appInst = new IcingaApplication();
	static_pointer_cast<ConfigObject>(appInst)->OnConfigLoaded();

	int rc = boost::unit_test::unit_test_main(&init_unit_test, argc, argv);

	appInst.reset();

	Application::Exit(rc);
}
