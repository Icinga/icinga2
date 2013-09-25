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

#include "icinga/cib.h"

using namespace icinga;

RingBuffer CIB::m_ActiveChecksStatistics(15 * 60);
RingBuffer CIB::m_PassiveChecksStatistics(15 * 60);

void CIB::UpdateActiveChecksStatistics(long tv, int num)
{
	m_ActiveChecksStatistics.InsertValue(tv, num);
}

int CIB::GetActiveChecksStatistics(long timespan)
{
	return m_ActiveChecksStatistics.GetValues(timespan);
}

void CIB::UpdatePassiveChecksStatistics(long tv, int num)
{
	m_PassiveChecksStatistics.InsertValue(tv, num);
}

int CIB::GetPassiveChecksStatistics(long timespan)
{
	return m_PassiveChecksStatistics.GetValues(timespan);
}
