/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/array.hpp"
#include "base/utility.hpp"
#include "base/objectlock.hpp"
#include "remote/url.hpp"
#include "remote/url-characters.hpp"
#include <boost/tokenizer.hpp>

using namespace icinga;

Url::Url(const String& base_url)
{
	String url = base_url;

	if (url.GetLength() == 0)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid URL Empty URL."));

	size_t pHelper = String::NPos;
	if (url[0] != '/')
		pHelper = url.Find(":");

	if (pHelper != String::NPos) {
		if (!ParseScheme(url.SubStr(0, pHelper)))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid URL Scheme."));
		url = url.SubStr(pHelper + 1);
	}

	if (*url.Begin() != '/')
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid URL: '/' expected after scheme."));

	if (url.GetLength() == 1) {
		return;
	}

	if (*(url.Begin() + 1) == '/') {
		pHelper = url.Find("/", 2);

		if (pHelper == String::NPos)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid URL: Missing '/' after authority."));

		if (!ParseAuthority(url.SubStr(0, pHelper)))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid URL Authority"));

		url = url.SubStr(pHelper);
	}

	if (*url.Begin() == '/') {
		pHelper = url.FindFirstOf("#?");
		if (!ParsePath(url.SubStr(1, pHelper - 1)))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid URL Path"));

		if (pHelper != String::NPos)
			url = url.SubStr(pHelper);
	} else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid URL: Missing path."));

	if (*url.Begin() == '?') {
		pHelper = url.Find("#");
		if (!ParseQuery(url.SubStr(1, pHelper - 1)))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid URL Query"));

		if (pHelper != String::NPos)
			url = url.SubStr(pHelper);
	}

	if (*url.Begin() == '#') {
		if (!ParseFragment(url.SubStr(1)))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid URL Fragment"));
	}
}

String Url::GetScheme() const
{
	return m_Scheme;
}

String Url::GetAuthority() const
{
	if (m_Host.IsEmpty())
		return "";

	String auth;
	if (!m_Username.IsEmpty()) {
		auth = m_Username;
		if (!m_Password.IsEmpty())
			auth += ":" + m_Password;
		auth += "@";
	}

	auth += m_Host;

	if (!m_Port.IsEmpty())
		auth += ":" + m_Port;

	return auth;
}

String Url::GetUsername() const
{
	return m_Username;
}

String Url::GetPassword() const
{
	return m_Password;
}

String Url::GetHost() const
{
	return m_Host;
}

String Url::GetPort() const
{
	return m_Port;
}

const std::vector<String>& Url::GetPath() const
{
	return m_Path;
}

const std::vector<std::pair<String, String>>& Url::GetQuery() const
{
	return m_Query;
}

String Url::GetFragment() const
{
	return m_Fragment;
}

void Url::SetScheme(const String& scheme)
{
	m_Scheme = scheme;
}

void Url::SetUsername(const String& username)
{
	m_Username = username;
}

void Url::SetPassword(const String& password)
{
	m_Password = password;
}

void Url::SetHost(const String& host)
{
	m_Host = host;
}

void Url::SetPort(const String& port)
{
	m_Port = port;
}

void Url::SetPath(const std::vector<String>& path)
{
	m_Path = path;
}

void Url::SetQuery(const std::vector<std::pair<String, String>>& query)
{
	m_Query = query;
}

void Url::SetArrayFormatUseBrackets(bool useBrackets)
{
	m_ArrayFormatUseBrackets = useBrackets;
}

void Url::AddQueryElement(const String& name, const String& value)
{
	m_Query.emplace_back(name, value);
}

void Url::SetFragment(const String& fragment) {
	m_Fragment = fragment;
}

String Url::Format(bool onlyPathAndQuery, bool printCredentials) const
{
	String url;

	if (!onlyPathAndQuery) {
		if (!m_Scheme.IsEmpty())
			url += m_Scheme + ":";

		if (printCredentials && !GetAuthority().IsEmpty())
			url += "//" + GetAuthority();
		else if (!GetHost().IsEmpty())
			url += "//" + GetHost() + (!GetPort().IsEmpty() ? ":" + GetPort() : "");
	}

	if (m_Path.empty())
		url += "/";
	else {
		for (const auto& segment : m_Path) {
			url += "/";
			url += Utility::EscapeString(segment, ACPATHSEGMENT_ENCODE, false);
		}
	}

	String param;
	if (!m_Query.empty()) {
		typedef std::pair<String, std::vector<String> > kv_pair;

		for (const auto& kv : m_Query) {
			String key = Utility::EscapeString(kv.first, ACQUERY_ENCODE, false);
			if (param.IsEmpty())
				param = "?";
			else
				param += "&";

			param += key;
			param += kv.second.IsEmpty() ?
				String() : "=" + Utility::EscapeString(kv.second, ACQUERY_ENCODE, false);
		}
	}

	url += param;

	if (!m_Fragment.IsEmpty())
		url += "#" + Utility::EscapeString(m_Fragment, ACFRAGMENT_ENCODE, false);

	return url;
}

bool Url::ParseScheme(const String& scheme)
{
	m_Scheme = scheme;

	if (scheme.FindFirstOf(ALPHA) != 0)
		return false;

	return (ValidateToken(scheme, ACSCHEME));
}

bool Url::ParseAuthority(const String& authority)
{
	String auth = authority.SubStr(2);
	size_t pos = auth.Find("@");
	if (pos != String::NPos && pos != 0) {
		if (!Url::ParseUserinfo(auth.SubStr(0, pos)))
			return false;
		auth = auth.SubStr(pos+1);
	}

	pos = auth.Find(":");
	if (pos != String::NPos) {
		if (pos == 0 || pos == auth.GetLength() - 1 || !Url::ParsePort(auth.SubStr(pos+1)))
			return false;
	}

	m_Host = auth.SubStr(0, pos);
	return ValidateToken(m_Host, ACHOST);
}

bool Url::ParseUserinfo(const String& userinfo)
{
	size_t pos = userinfo.Find(":");
	m_Username = userinfo.SubStr(0, pos);
	if (!ValidateToken(m_Username, ACUSERINFO))
		return false;
	m_Username = Utility::UnescapeString(m_Username);
	if (pos != String::NPos && pos != userinfo.GetLength() - 1) {
		m_Password = userinfo.SubStr(pos+1);
		if (!ValidateToken(m_Username, ACUSERINFO))
			return false;
		m_Password = Utility::UnescapeString(m_Password);
	} else
		m_Password = "";

	return true;
}

bool Url::ParsePort(const String& port)
{
	m_Port = Utility::UnescapeString(port);
	if (!ValidateToken(m_Port, ACPORT))
		return false;
	return true;
}

bool Url::ParsePath(const String& path)
{
	const std::string& pathStr = path;
	boost::char_separator<char> sep("/");
	boost::tokenizer<boost::char_separator<char> > tokens(pathStr, sep);

	for (const auto& token : tokens) {
		if (token.IsEmpty())
			continue;

		if (!ValidateToken(token, ACPATHSEGMENT))
			return false;

		m_Path.emplace_back(Utility::UnescapeString(token));
	}

	return true;
}

bool Url::ParseQuery(const String& query)
{
	/* Tokenizer does not like String AT ALL */
	const std::string& queryStr = query;
	boost::char_separator<char> sep("&");
	boost::tokenizer<boost::char_separator<char> > tokens(queryStr, sep);

	for (const auto& token : tokens) {
		size_t pHelper = token.Find("=");

		if (pHelper == 0)
			// /?foo=bar&=bar == invalid
			return false;

		String key = token.SubStr(0, pHelper);
		String value = Empty;

		if (pHelper != String::NPos && pHelper != token.GetLength() - 1)
			value = token.SubStr(pHelper+1);

		if (!ValidateToken(value, ACQUERY))
			return false;

		value = Utility::UnescapeString(value);

		pHelper = key.Find("[]");

		if (pHelper == 0 || (pHelper != String::NPos && pHelper != key.GetLength()-2))
			return false;

		key = key.SubStr(0, pHelper);

		if (!ValidateToken(key, ACQUERY))
			return false;

		m_Query.emplace_back(Utility::UnescapeString(key), std::move(value));
	}

	return true;
}

bool Url::ParseFragment(const String& fragment)
{
	m_Fragment = Utility::UnescapeString(fragment);

	return ValidateToken(fragment, ACFRAGMENT);
}

bool Url::ValidateToken(const String& token, const String& symbols)
{
	for (const char ch : token) {
		if (symbols.FindFirstOf(ch) == String::NPos)
			return false;
	}

	return true;
}

