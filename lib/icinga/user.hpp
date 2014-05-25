/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/i2-icinga.hpp"
#include "icinga/user.thpp"
#include "icinga/timeperiod.hpp"
#include "remote/messageorigin.hpp"

namespace icinga
{

/**
 * A User.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API User : public ObjectImpl<User>
{
public:
	DECLARE_PTR_TYPEDEFS(User);
	DECLARE_TYPENAME(User);

	void AddGroup(const String& name);

	/* Notifications */
	TimePeriod::Ptr GetPeriod(void) const;

	static void ValidateFilters(const String& location, const Dictionary::Ptr& attrs);

	int GetModifiedAttributes(void) const;
	void SetModifiedAttributes(int flags, const MessageOrigin& origin = MessageOrigin());

protected:
	virtual void Stop(void);

	virtual void OnConfigLoaded(void);
private:
	mutable boost::mutex m_UserMutex;
};

}

#endif /* USER_H */
