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

#include "base/array.hpp"
#include "base/utility.hpp"
#include "base/objectlock.hpp"
#include "remote/url.hpp"
#include "remote/url-characters.hpp"
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

Url::Url(const String& base_url)
{
	String url = base_url;

	if (url.GetLength() == 0)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid URL Empty URL."));

	size_t pHelper = url.Find(":");

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

String Url::GetScheme(void) const
{
	return m_Scheme;
}

String Url::GetAuthority(void) const
{
	return m_Authority;
}

const std::vector<String>& Url::GetPath(void) const
{
	return m_Path;
}

const std::map<String, std::vector<String> >& Url::GetQuery(void) const
{
	return m_Query;
}

String Url::GetQueryElement(const String& name) const
{
	std::map<String, std::vector<String> >::const_iterator it = m_Query.find(name);

	if (it == m_Query.end())
		return String();

	return it->second.back();
}

const std::vector<String>& Url::GetQueryElements(const String& name) const
{
	std::map<String, std::vector<String> >::const_iterator it = m_Query.find(name);

	if (it == m_Query.end()) {
		static std::vector<String> emptyVector;
		return emptyVector;
	}

	return it->second;
}

String Url::GetFragment(void) const
{
	return m_Fragment;
}

String Url::Format(void) const
{
	String url;

	if (!m_Scheme.IsEmpty())
		url += m_Scheme + ":";

	if (!m_Authority.IsEmpty())
		url += "//" + m_Authority;

	if (m_Path.empty())
		url += "/";
	else {
		BOOST_FOREACH (const String p, m_Path) {
			url += "/";
			url += Utility::EscapeString(p, ACPATHSEGMENT, false);
		}
	}

	String param;
	if (!m_Query.empty()) {
		typedef std::pair<String, std::vector<String> > kv_pair;

		BOOST_FOREACH (const kv_pair& kv, m_Query) {
			String key = Utility::EscapeString(kv.first, ACQUERY, false);
			if (param.IsEmpty())
				param = "?";
			else
				param += "&";
			
			String temp;
			BOOST_FOREACH (const String s, kv.second) {
				if (!temp.IsEmpty())
					temp += "&";

				temp += key + "[]=" + Utility::EscapeString(s, ACQUERY, false);
			}
			param += temp;
		}
	}

	url += param;

	if (!m_Fragment.IsEmpty())
		url += "#" + Utility::EscapeString(m_Fragment, ACFRAGMENT, false);

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
	m_Authority = authority.SubStr(2);
	//Just safe the Authority and don't care about the details
	return (ValidateToken(m_Authority, ACHOST GEN_DELIMS));
}

bool Url::ParsePath(const String& path)
{
	std::string pathStr = path;
	boost::char_separator<char> sep("/");
	boost::tokenizer<boost::char_separator<char> > tokens(pathStr, sep);

	BOOST_FOREACH(const String& token, tokens) {
		if (token.IsEmpty())
			continue;

		if (!ValidateToken(token, ACPATHSEGMENT))
			return false;

		String decodedToken = Utility::UnescapeString(token);

		m_Path.push_back(decodedToken);
	}

	return true;
}

bool Url::ParseQuery(const String& query)
{
	//Tokenizer does not like String AT ALL
	std::string queryStr = query;
	boost::char_separator<char> sep("&");
	boost::tokenizer<boost::char_separator<char> > tokens(queryStr, sep);

	BOOST_FOREACH(const String& token, tokens) {
		size_t pHelper = token.Find("=");

		if (pHelper == 0)
			// /?foo=bar&=bar == invalid
			return false;
		
		String key = token.SubStr(0, pHelper);
		String value = Empty;

		if (pHelper != token.GetLength()-1)
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

		key = Utility::UnescapeString(key);

		std::map<String, std::vector<String> >::iterator it = m_Query.find(key);

		if (it == m_Query.end()) {
			m_Query[key] = std::vector<String>();
			m_Query[key].push_back(value);
		} else
			m_Query[key].push_back(value);
		
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
	BOOST_FOREACH (const char c, token.CStr()) {
		if (symbols.FindFirstOf(c) == String::NPos)
			return false;
	}

	return true;
}
