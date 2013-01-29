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

#ifndef DOWNTIMEPROCESSOR_H
#define DOWNTIMEPROCESSOR_H

namespace icinga
{

/**
 * Common Information Base class. Holds some statistics (and will likely be
 * removed/refactored).
 *
 * @ingroup icinga
 */
class I2_ICINGA_API DowntimeProcessor
{
public:
	static int GetNextDowntimeID(void);

	static int AddDowntime(const DynamicObject::Ptr& owner,
	    const String& author, const String& comment,
	    double startTime, double endTime,
	    bool fixed, int triggeredBy, double duration);

	static void RemoveDowntime(int id);

	static DynamicObject::Ptr GetOwnerByDowntimeID(int id);
	static Dictionary::Ptr GetDowntimeByID(int id);

	static bool IsDowntimeActive(const Dictionary::Ptr& downtime);

	static void InvalidateDowntimeCache(void);

private:
	static int m_NextDowntimeID;

	static map<int, DynamicObject::WeakPtr> m_DowntimeCache;
	static bool m_DowntimeCacheValid;

	DowntimeProcessor(void);

	static void AddDowntimesToCache(const DynamicObject::Ptr& owner);
	static void ValidateDowntimeCache(void);
};

}

#endif /* DOWNTIMEPROCESSOR_H */
