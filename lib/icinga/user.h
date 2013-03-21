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

#ifndef USER_H
#define USER_H

#include "icinga/i2-icinga.h"
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
class I2_ICINGA_API User : public DynamicObject
{
public:
	typedef shared_ptr<User> Ptr;
	typedef weak_ptr<User> WeakPtr;

	explicit User(const Dictionary::Ptr& serializedUpdate);
	~User(void);

	static User::Ptr GetByName(const String& name);

	String GetDisplayName(void) const;
	Array::Ptr GetGroups(void) const;
	TimePeriod::Ptr GetNotificationPeriod(void) const;

	Dictionary::Ptr GetMacros(void) const;
	Dictionary::Ptr CalculateDynamicMacros(void) const;

protected:
	virtual void OnAttributeChanged(const String& name);

private:
	Attribute<String> m_DisplayName;
	Attribute<Dictionary::Ptr> m_Macros;
	Attribute<String> m_NotificationPeriod;
	Attribute<Array::Ptr> m_Groups;
};

}

#endif /* USER_H */
