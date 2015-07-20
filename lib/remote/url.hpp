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

#ifndef URL_H
#define URL_H

#include "remote/i2-remote.hpp"
#include "base/object.hpp"
#include "base/string.hpp"
#include "base/value.hpp"
#include <map>
#include <vector>

namespace icinga
{

/**
 * A url class to use with the API
 *
 * @ingroup base
 */
class I2_REMOTE_API Url : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Url);

	Url(const String& url);

	String Format(void) const;

	String GetScheme(void) const;
	String GetAuthority(void) const;
	const std::vector<String>& GetPath(void) const;
	const std::map<String,Value>& GetQuery(void) const;
	Value GetQueryElement(const String& name) const;
	String GetFragment(void) const;

private:
	String m_Scheme;
	String m_Authority;
	std::vector<String> m_Path;
	std::map<String,Value> m_Query;
	String m_Fragment;

	bool ParseScheme(const String& scheme);
	bool ParseAuthority(const String& authority);
	bool ParsePath(const String& path);
	bool ParseQuery(const String& query);
	bool ParseFragment(const String& fragment);

	static bool ValidateToken(const String& token, const String& symbols);
};

}
#endif /* URL_H */
