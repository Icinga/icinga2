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

#ifndef ICINGASTATUSWRITER_H
#define ICINGASTATUSWRITER_H

#include "icinga/icingastatuswriter.thpp"
#include "base/timer.hpp"

namespace icinga
{

/**
 * @ingroup compat
 */
class IcingaStatusWriter : public ObjectImpl<IcingaStatusWriter>
{
public:
	DECLARE_OBJECT(IcingaStatusWriter);
	DECLARE_OBJECTNAME(IcingaStatusWriter);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);
	static Dictionary::Ptr GetStatusData(void);

protected:
	virtual void Start(void);

private:
	Timer::Ptr m_StatusTimer;
	void StatusTimerHandler(void);
};

}

#endif /* ICINGASTATUSWRITER_H */
