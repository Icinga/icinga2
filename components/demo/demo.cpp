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

#include "demo/demo.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;

REGISTER_TYPE(Demo);

/**
 * Starts the component.
 */
void Demo::Start(void)
{
	DynamicObject::Start();

	m_DemoTimer = boost::make_shared<Timer>();
	m_DemoTimer->SetInterval(5);
	m_DemoTimer->OnTimerExpired.connect(boost::bind(&Demo::DemoTimerHandler, this));
	m_DemoTimer->Start();
}

/**
 * Stops the component.
 */
void Demo::Stop(void)
{
	/* Nothing to do here. */
}

/**
 * Periodically sends a demo::HelloWorld message.
 *
 * @param - Event arguments for the timer.
 */
void Demo::DemoTimerHandler(void)
{
	Log(LogInformation, "demo", "Hello World!");
}
