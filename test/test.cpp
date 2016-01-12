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

#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE icinga2_test

#include "base/application.hpp"
#include "base/timer.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

struct InitLibBase
{
	InitLibBase(void)
	{
		Application::InitializeBase();
	}

	~InitLibBase(void)
	{
		Application::UninitializeBase();
	}
};

BOOST_GLOBAL_FIXTURE(InitLibBase);
