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

#ifndef USER_H
#define USER_H

#include "icinga/i2-icinga.h"
#include "icinga/macroresolver.h"
#include "icinga/timeperiod.h"
#include "base/dynamicobject.h"
#include "base/array.h"

namespace icinga
{

/**
 * A User.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API User : public DynamicObject, public MacroResolver
{
public:
	DECLARE_PTR_TYPEDEFS(User);
	DECLARE_TYPENAME(User);

	String GetDisplayName(void) const;
	Array::Ptr GetGroups(void) const;

	/* Notifications */
	bool GetEnableNotifications(void) const;
	void SetEnableNotifications(bool enabled);
	TimePeriod::Ptr GetNotificationPeriod(void) const;
 	unsigned long GetNotificationTypeFilter(void) const;
	unsigned long GetNotificationStateFilter(void) const;
	void SetLastNotification(double ts);
	double GetLastNotification(void) const;

	Dictionary::Ptr GetMacros(void) const;

	virtual bool ResolveMacro(const String& macro, const Dictionary::Ptr& cr, String *result) const;

protected:
	virtual void Stop(void);

	virtual void OnConfigLoaded(void);

	virtual void InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const;
	virtual void InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes);

private:
	String m_DisplayName;
	Dictionary::Ptr m_Macros;
	Array::Ptr m_Groups;
	Value m_EnableNotifications;
	String m_NotificationPeriod;
	Value m_NotificationTypeFilter;
	Value m_NotificationStateFilter;
	double m_LastNotification;
};

}

#endif /* USER_H */
