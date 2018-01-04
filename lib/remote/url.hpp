/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include "base/array.hpp"
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
class Url final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Url);

	Url() = default;
	Url(const String& url);

	String Format(bool onlyPathAndQuery = false, bool printCredentials = false) const;

	String GetScheme() const;
	String GetAuthority() const;
	String GetUsername() const;
	String GetPassword() const;
	String GetHost() const;
	String GetPort() const;
	const std::vector<String>& GetPath() const;
	const std::map<String, std::vector<String> >& GetQuery() const;
	String GetQueryElement(const String& name) const;
	const std::vector<String>& GetQueryElements(const String& name) const;
	String GetFragment() const;

	void SetScheme(const String& scheme);
	void SetUsername(const String& username);
	void SetPassword(const String& password);
	void SetHost(const String& host);
	void SetPort(const String& port);
	void SetPath(const std::vector<String>& path);
	void SetQuery(const std::map<String, std::vector<String> >& query);

	void AddQueryElement(const String& name, const String& query);
	void SetQueryElements(const String& name, const std::vector<String>& query);
	void SetFragment(const String& fragment);

private:
	String m_Scheme;
	String m_Username;
	String m_Password;
	String m_Host;
	String m_Port;
	std::vector<String> m_Path;
	std::map<String, std::vector<String> > m_Query;
	String m_Fragment;

	bool ParseScheme(const String& scheme);
	bool ParseAuthority(const String& authority);
	bool ParseUserinfo(const String& userinfo);
	bool ParsePort(const String& port);
	bool ParsePath(const String& path);
	bool ParseQuery(const String& query);
	bool ParseFragment(const String& fragment);

	static bool ValidateToken(const String& token, const String& symbols);
};

}
#endif /* URL_H */
