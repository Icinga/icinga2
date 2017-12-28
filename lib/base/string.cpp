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

#include "base/string.hpp"
#include "base/value.hpp"
#include "base/primitivetype.hpp"
#include "base/dictionary.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <ostream>

using namespace icinga;

template class std::vector<String>;

REGISTER_BUILTIN_TYPE(String, String::GetPrototype());

const String::SizeType String::NPos = std::string::npos;

String::String(void)
	: m_Data()
{ }

String::String(const char *data)
	: m_Data(data)
{ }

String::String(const std::string& data)
	: m_Data(data)
{ }

String::String(std::string&& data)
	: m_Data(std::move(data))
{ }

String::String(String::SizeType n, char c)
	: m_Data(n, c)
{ }

String::String(const String& other)
	: m_Data(other)
{ }

String::~String(void)
{ }

String& String::operator=(const String& other)
{
	m_Data = other.m_Data;
	return *this;
}

const char& String::operator[](String::SizeType pos) const
{
	return m_Data[pos];
}

bool String::IsEmpty(void) const
{
	return m_Data.empty();
}

bool String::operator<(const String& rhs) const
{
	return m_Data < rhs.m_Data;
}

String::operator const std::string&(void) const
{
	return m_Data;
}

const char *String::CStr(void) const
{
	return m_Data.c_str();
}

String::SizeType String::GetLength(void) const
{
	return m_Data.size();
}

const std::string& String::GetData(void) const
{
	return m_Data;
}

std::vector<String> String::Split(const char *separators) const
{
	std::vector<String> result;
	boost::algorithm::split(result, m_Data, boost::is_any_of(separators));
	return result;
}

String String::Trim(void) const
{
	std::string t = m_Data;
	boost::algorithm::trim(t);
	return t;
}

String String::ToLower(void) const
{
	std::string t = m_Data;
	boost::algorithm::to_lower(t);
	return t;
}

String String::ToUpper(void) const
{
	std::string t = m_Data;
	boost::algorithm::to_upper(t);
	return t;
}

String::SizeType String::Find(const String& str, String::SizeType pos) const
{
	return m_Data.find(str, pos);
}

String::SizeType String::RFind(const String& str, String::SizeType pos) const
{
	return m_Data.rfind(str, pos);
}

String::SizeType String::FindFirstOf(const char *s, String::SizeType pos) const
{
	return m_Data.find_first_of(s, pos);
}

String::SizeType String::FindFirstOf(char ch, String::SizeType pos) const
{
	return m_Data.find_first_of(ch, pos);
}

String::SizeType String::FindFirstNotOf(const char *s, String::SizeType pos) const
{
	return m_Data.find_first_not_of(s, pos);
}

String::SizeType String::FindFirstNotOf(char ch, String::SizeType pos) const
{
	return m_Data.find_first_not_of(ch, pos);
}

String::SizeType String::FindLastOf(const char *s, String::SizeType pos) const
{
	return m_Data.find_last_of(s, pos);
}

String::SizeType String::FindLastOf(char ch, String::SizeType pos) const
{
	return m_Data.find_last_of(ch, pos);
}

String String::SubStr(String::SizeType first, String::SizeType len) const
{
	return m_Data.substr(first, len);
}

String String::Reverse(void) const
{
	return String(m_Data.rbegin(), m_Data.rend());
}

bool String::Contains(const String& str) const
{
	return (m_Data.find(str) != std::string::npos);
}

String::ConstIterator String::Begin(void) const
{
	return m_Data.begin();
}

String::ConstIterator String::End(void) const
{
	return m_Data.end();
}

String::ConstReverseIterator String::RBegin(void) const
{
	return m_Data.rbegin();
}

String::ConstReverseIterator String::REnd(void) const
{
	return m_Data.rend();
}

String& String::operator=(String&& rhs)
{
	m_Data = std::move(rhs.m_Data);
	return *this;
}

String& String::operator=(const std::string& rhs)
{
	m_Data = rhs;
	return *this;
}

String& String::operator=(const char *rhs)
{
	m_Data = rhs;
	return *this;
}

const char& String::operator[](String::SizeType pos) const
{
	return m_Data[pos];
}

char& String::operator[](String::SizeType pos)
{
	return m_Data[pos];
}

String& String::operator+=(const String& rhs)
{
	m_Data += rhs.m_Data;
	return *this;
}

String& String::operator+=(const char *rhs)
{
	m_Data += rhs;
	return *this;
}

String& String::operator+=(char rhs)
{
	m_Data += rhs;
	return *this;
}

bool String::IsEmpty(void) const
{
	return m_Data.empty();
}

bool String::operator<(const String& rhs) const
{
	return m_Data < rhs.m_Data;
}

String::operator const std::string&(void) const
{
	return m_Data;
}

String::SizeType String::Find(const String& str, String::SizeType pos) const
{
	return m_Data.find(str, pos);
}

String::SizeType String::RFind(const String& str, String::SizeType pos) const
{
	return m_Data.rfind(str, pos);
}

String::SizeType String::FindFirstOf(const char *s, String::SizeType pos) const
{
	return m_Data.find_first_of(s, pos);
}

String::SizeType String::FindFirstOf(char ch, String::SizeType pos) const
{
	return m_Data.find_first_of(ch, pos);
}

String::SizeType String::FindFirstNotOf(const char *s, String::SizeType pos) const
{
	return m_Data.find_first_not_of(s, pos);
}

String::SizeType String::FindFirstNotOf(char ch, String::SizeType pos) const
{
	return m_Data.find_first_not_of(ch, pos);
}

String::SizeType String::FindLastOf(const char *s, String::SizeType pos) const
{
	return m_Data.find_last_of(s, pos);
}

String::SizeType String::FindLastOf(char ch, String::SizeType pos) const
{
	return m_Data.find_last_of(ch, pos);
}

String String::SubStr(String::SizeType first, String::SizeType len) const
{
	return m_Data.substr(first, len);
}

String String::ReplaceAll(const String& srch, const String& rep) const
{
	std::string t = m_Data;
	boost::algorithm::replace_all(t, srch, rep);
	return t;
}

String String::ReplaceAll(const std::initializer_list<String>& srch, const std::initializer_list<String>& rep) const
{
	std::string t = m_Data;
	ASSERT(srch.size() == rep.size());
	for (int i = 0; i < srch.size(); i++)
		boost::algorithm::replace_all(t, *(srch.begin() + i), *(rep.begin() + i));
	return t;
}

String::operator double(void) const
{
	try {
		std::size_t pos;
		double res = std::stod(m_Data, &pos);
		if (pos < m_Data.size())
			throw std::invalid_argument("m_Datao");
		return res;
	} catch (const std::exception&) {
		std::ostringstream msgbuf;
		msgbuf << "Can't convert '" << m_Data << "' to a floating point number.";
		BOOST_THROW_EXCEPTION(std::invalid_argument(msgbuf.str()));
	}
}

String::operator int(void) const
{
	return static_cast<double>(*this);
}

String::ConstIterator icinga::range_begin(const String& x)
{
	return x.Begin();
}

String::ConstIterator icinga::range_end(const String& x)
{
	return x.End();
}
