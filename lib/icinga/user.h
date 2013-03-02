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

	User(const Dictionary::Ptr& properties);
	~User(void);

	static User::Ptr GetByName(const String& name);

	String GetDisplayName(void) const;
	Dictionary::Ptr GetGroups(void) const;

	Dictionary::Ptr GetMacros(void) const;
	Dictionary::Ptr CalculateDynamicMacros(void) const;

protected:
	void OnAttributeChanged(const String& name, const Value& oldValue);

private:
	Attribute<String> m_DisplayName;
	Attribute<Dictionary::Ptr> m_Macros;
	Attribute<Dictionary::Ptr> m_Groups;
};

}

#endif /* USER_H */
