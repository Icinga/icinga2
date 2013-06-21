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

#include "icinga/service.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/timer.h"
#include "base/utility.h"
#include <boost/tuple/tuple.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

#define FLAPPING_INTERVAL (30 * 60)

void Service::UpdateFlappingStatus(bool stateChange)
{
	double ts, now;
	long counter;

	now = Utility::GetTime();

	if (m_FlappingLastChange.IsEmpty()) {
		ts = now;
		counter = 0;
	} else {
		ts = m_FlappingLastChange;
		counter = m_FlappingCounter;
	}

	double diff = now - ts;

	if (diff > 0)
		counter -= 0.5 * m_FlappingCounter / (diff / FLAPPING_INTERVAL);

	if (stateChange)
		counter += diff;

	m_FlappingCounter = counter;
	Touch("flapping_counter");

	m_FlappingLastChange = now;
	Touch("flapping_lastchange");
}

bool Service::IsFlapping(void) const
{
	double threshold = 30;

	if (!m_FlappingThreshold.IsEmpty())
		threshold = m_FlappingThreshold;

	if (m_FlappingCounter.IsEmpty())
		return false;

	long counter = m_FlappingCounter;

	return (counter > threshold * FLAPPING_INTERVAL / 100);

}
