/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "remote/i2-remote.hpp"
#include "base/object.hpp"
#include "base/string.hpp"
#include "base/array.hpp"
#include "base/value.hpp"
#include <map>
#include <utility>
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
	const std::vector<std::pair<String, String>>& GetQuery() const;
	String GetFragment() const;

	void SetScheme(const String& scheme);
	void SetUsername(const String& username);
	void SetPassword(const String& password);
	void SetHost(const String& host);
	void SetPort(const String& port);
	void SetPath(const std::vector<String>& path);
	void SetQuery(const std::vector<std::pair<String, String>>& query);
	void SetArrayFormatUseBrackets(bool useBrackets = true);

	void AddQueryElement(const String& name, const String& query);
	void SetFragment(const String& fragment);

private:
	String m_Scheme;
	String m_Username;
	String m_Password;
	String m_Host;
	String m_Port;
	std::vector<String> m_Path;
	std::vector<std::pair<String, String>> m_Query;
	bool m_ArrayFormatUseBrackets;
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
