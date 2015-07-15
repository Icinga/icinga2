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

#include "base/url.hpp"
#include "base/url-characters.hpp"
#include "base/array.hpp"
#include "base/utility.hpp"
#include "base/objectlock.hpp"
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

Url::Url(const String& base_url)
{
	String url = base_url;

	if (url.GetLength() == 0)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid URL Empty URL."));

	size_t pHelper = url.Find(":");

	if (pHelper == String::NPos) {
		m_Scheme = "";
	} else {
		if (!ParseScheme(url.SubStr(0, pHelper)))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid URL Scheme."));
		url = url.SubStr(pHelper + 1);
	}

	if (*url.Begin() != '/')
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid URL: '/' expected after scheme."));

	if (url.GetLength() == 1) {
		return;
	}

	if (*(url.Begin() + 1) != '/')
		m_Authority = "";
	else {
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

const std::map<String,Value>& Url::GetQuery(void) const
{
	return m_Query;
}

Value Url::GetQueryElement(const String& name) const
{
	std::map<String, Value>::const_iterator it = m_Query.find(name);

	if (it == m_Query.end())
		return Empty;

	return it->second;
}


String Url::GetFragment(void) const
{
	return m_Fragment;
}

String Url::Format(void) const
{
	String url = "";

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

	String param = "";
	if (!m_Query.empty()) {
		typedef std::pair<String, Value> kv_pair;

		BOOST_FOREACH (const kv_pair& kv, m_Query) {
			String key = Utility::EscapeString(kv.first, ACQUERY, false);
			if (param.IsEmpty())
				param = "?";
			else
				param += "&";

			Value val = kv.second;

			if (val.IsEmpty())
				param += key;
			else {
				if (val.IsObjectType<Array>()) {
					Array::Ptr arr = val;
					String temp = "";

					ObjectLock olock(arr);
					BOOST_FOREACH (const String& sArrIn, arr) {
						if (!temp.IsEmpty())
							temp += "&";

						temp += key + "[]=" + Utility::EscapeString(sArrIn, ACQUERY, false);
					}

					param += temp;
				} else
					param += key + "=" + Utility::EscapeString(kv.second, ACQUERY, false);
			}
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
	//TODO parse all Authorities
	m_Authority = authority.SubStr(2);
	return (ValidateToken(m_Authority, ACHOST));
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

		String key = token.SubStr(0, pHelper);
		String value = Empty;

		if (pHelper != String::NPos) {
			if (pHelper == token.GetLength() - 1)
				return false;

			value = token.SubStr(pHelper + 1);
			if (!ValidateToken(value, ACQUERY))
				return false;
			else
				value = Utility::UnescapeString(value);
		} else
			String key = token;

		if (key.IsEmpty())
			return false;

		pHelper = key.Find("[]");

		if (pHelper != String::NPos) {

			if (key.GetLength() < 3)
				return false;

			key = key.SubStr(0, key.GetLength() - 2);
			key = Utility::UnescapeString(key);

			if (!ValidateToken(value, ACQUERY))
				return false;

			std::map<String, Value>::iterator it = m_Query.find(key);

			if (it == m_Query.end()) {
				Array::Ptr tmp = new Array();
				tmp->Add(Utility::UnescapeString(value));
				m_Query[key] = tmp;
			} else if (m_Query[key].IsObjectType<Array>()){
				Array::Ptr arr = it->second;
				arr->Add(Utility::UnescapeString(value));
			} else
				return false;
		} else {
			key = Utility::UnescapeString(key);

			if (m_Query.find(key) == m_Query.end() && ValidateToken(key, ACQUERY))
				m_Query[key] = Utility::UnescapeString(value);
			else
				return false;
		}
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
