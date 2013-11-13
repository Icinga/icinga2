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

#ifndef SCHEDULEDDOWNTIME_H
#define SCHEDULEDDOWNTIME_H

#include "icinga/i2-icinga.h"
#include "icinga/scheduleddowntime.th"
#include "icinga/service.h"
#include <utility>

namespace icinga
{

/**
 * An Icinga scheduled downtime specification.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API ScheduledDowntime : public ObjectImpl<ScheduledDowntime>
{
public:
	DECLARE_PTR_TYPEDEFS(ScheduledDowntime);
	DECLARE_TYPENAME(ScheduledDowntime);

	static void StaticInitialize(void);

	Service::Ptr GetService(void) const;

protected:
	virtual void Start(void);

private:
	static void TimerProc(void);

	std::pair<double, double> FindNextSegment(void);
	void CreateNextDowntime(void);
};

}

#endif /* SCHEDULEDDOWNTIME_H */
