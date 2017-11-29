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

#include "base/timer.hpp"
#include "base/utility.hpp"
#include "base/application.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_timer)

BOOST_AUTO_TEST_CASE(construct)
{
	Timer::Ptr timer = new Timer();
	BOOST_CHECK(timer);
}

BOOST_AUTO_TEST_CASE(interval)
{
	Timer::Ptr timer = new Timer();
	timer->SetInterval(1.5);
	BOOST_CHECK(timer->GetInterval() == 1.5);
}

static void Callback(int *counter)
{
	(*counter)++;
}

BOOST_AUTO_TEST_CASE(invoke)
{
	int counter;
	Timer::Ptr timer = new Timer();
	timer->OnTimerExpired.connect(std::bind(&Callback, &counter));
	timer->SetInterval(1);

	counter = 0;
	timer->Start();
	Utility::Sleep(5.5);
	timer->Stop();

	BOOST_CHECK(counter >= 4 && counter <= 6);
}

BOOST_AUTO_TEST_CASE(scope)
{
	int counter;
	Timer::Ptr timer = new Timer();
	timer->OnTimerExpired.connect(std::bind(&Callback, &counter));
	timer->SetInterval(1);

	counter = 0;
	timer->Start();
	Utility::Sleep(5.5);
	timer.reset();
	Utility::Sleep(5.5);

	BOOST_CHECK(counter >= 4 && counter <= 6);
}

BOOST_AUTO_TEST_SUITE_END()
