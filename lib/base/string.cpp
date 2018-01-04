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
#include <ostream>

using namespace icinga;

template class std::vector<String>;

REGISTER_BUILTIN_TYPE(String, String::GetPrototype());

const String::SizeType String::NPos = std::string::npos;

String::String()
	: m_Data()
{ }

String::String(const char *data)
	: m_Data(data)
{ }

String::String(std::string data)
	: m_Data(std::move(data))
{ }

String::String(String::SizeType n, char c)
	: m_Data(n, c)
{ }

String::String(const String& other)
	: m_Data(other)
{ }

String::String(String&& other)
	: m_Data(std::move(other.m_Data))
{ }

#ifndef _MSC_VER
String::String(Value&& other)
{
	*this = std::move(other);
}
#endif /* _MSC_VER */

String::~String()
{ }

String& String::operator=(Value&& other)
{
	if (other.IsString())
		m_Data = std::move(other.Get<String>());
	else
		*this = static_cast<String>(other);

	return *this;
}

String& String::operator+=(const Value& rhs)
{
	m_Data += static_cast<String>(rhs);
	return *this;
}

String& String::operator=(const String& rhs)
{
	m_Data = rhs.m_Data;
	return *this;
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

bool String::IsEmpty() const
{
	return m_Data.empty();
}

bool String::operator<(const String& rhs) const
{
	return m_Data < rhs.m_Data;
}

String::operator const std::string&() const
{
	return m_Data;
}

const char *String::CStr() const
{
	return m_Data.c_str();
}

void String::Clear()
{
	m_Data.clear();
}

String::SizeType String::GetLength() const
{
	return m_Data.size();
}

std::string& String::GetData()
{
	return m_Data;
}

const std::string& String::GetData() const
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

std::vector<String> String::Split(const char *separators) const
{
	std::vector<String> result;
	boost::algorithm::split(result, m_Data, boost::is_any_of(separators));
	return result;
}

void String::Replace(String::SizeType first, String::SizeType second, const String& str)
{
	m_Data.replace(first, second, str);
}

String String::Trim() const
{
	String t = m_Data;
	boost::algorithm::trim(t);
	return t;
}

String String::ToLower() const
{
	String t = m_Data;
	boost::algorithm::to_lower(t);
	return t;
}

String String::ToUpper() const
{
	String t = m_Data;
	boost::algorithm::to_upper(t);
	return t;
}

String String::Reverse() const
{
	String t = m_Data;
	std::reverse(t.m_Data.begin(), t.m_Data.end());
	return t;
}

void String::Append(int count, char ch)
{
	m_Data.append(count, ch);
}

bool String::Contains(const String& str) const
{
	return (m_Data.find(str) != std::string::npos);
}

void String::swap(String& str)
{
	m_Data.swap(str.m_Data);
}

String::Iterator String::erase(String::Iterator first, String::Iterator last)
{
	return m_Data.erase(first, last);
}

String::Iterator String::Begin()
{
	return m_Data.begin();
}

String::ConstIterator String::Begin() const
{
	return m_Data.begin();
}

String::Iterator String::End()
{
	return m_Data.end();
}

String::ConstIterator String::End() const
{
	return m_Data.end();
}

String::ReverseIterator String::RBegin()
{
	return m_Data.rbegin();
}

String::ConstReverseIterator String::RBegin() const
{
	return m_Data.rbegin();
}

String::ReverseIterator String::REnd()
{
	return m_Data.rend();
}

String::ConstReverseIterator String::REnd() const
{
	return m_Data.rend();
}

std::ostream& icinga::operator<<(std::ostream& stream, const String& str)
{
	stream << str.GetData();
	return stream;
}

std::istream& icinga::operator>>(std::istream& stream, String& str)
{
	std::string tstr;
	stream >> tstr;
	str = tstr;
	return stream;
}

String icinga::operator+(const String& lhs, const String& rhs)
{
	return lhs.GetData() + rhs.GetData();
}

String icinga::operator+(const String& lhs, const char *rhs)
{
	return lhs.GetData() + rhs;
}

String icinga::operator+(const char *lhs, const String& rhs)
{
	return lhs + rhs.GetData();
}

bool icinga::operator==(const String& lhs, const String& rhs)
{
	return lhs.GetData() == rhs.GetData();
}

bool icinga::operator==(const String& lhs, const char *rhs)
{
	return lhs.GetData() == rhs;
}

bool icinga::operator==(const char *lhs, const String& rhs)
{
	return lhs == rhs.GetData();
}

bool icinga::operator<(const String& lhs, const char *rhs)
{
	return lhs.GetData() < rhs;
}

bool icinga::operator<(const char *lhs, const String& rhs)
{
	return lhs < rhs.GetData();
}

bool icinga::operator>(const String& lhs, const String& rhs)
{
	return lhs.GetData() > rhs.GetData();
}

bool icinga::operator>(const String& lhs, const char *rhs)
{
	return lhs.GetData() > rhs;
}

bool icinga::operator>(const char *lhs, const String& rhs)
{
	return lhs > rhs.GetData();
}

bool icinga::operator<=(const String& lhs, const String& rhs)
{
	return lhs.GetData() <= rhs.GetData();
}

bool icinga::operator<=(const String& lhs, const char *rhs)
{
	return lhs.GetData() <= rhs;
}

bool icinga::operator<=(const char *lhs, const String& rhs)
{
	return lhs <= rhs.GetData();
}

bool icinga::operator>=(const String& lhs, const String& rhs)
{
	return lhs.GetData() >= rhs.GetData();
}

bool icinga::operator>=(const String& lhs, const char *rhs)
{
	return lhs.GetData() >= rhs;
}

bool icinga::operator>=(const char *lhs, const String& rhs)
{
	return lhs >= rhs.GetData();
}

bool icinga::operator!=(const String& lhs, const String& rhs)
{
	return lhs.GetData() != rhs.GetData();
}

bool icinga::operator!=(const String& lhs, const char *rhs)
{
	return lhs.GetData() != rhs;
}

bool icinga::operator!=(const char *lhs, const String& rhs)
{
	return lhs != rhs.GetData();
}

String::Iterator icinga::begin(String& x)
{
	return x.Begin();
}

String::ConstIterator icinga::begin(const String& x)
{
	return x.Begin();
}

String::Iterator icinga::end(String& x)
{
	return x.End();
}

String::ConstIterator icinga::end(const String& x)
{
	return x.End();
}
String::Iterator icinga::range_begin(String& x)
{
	return x.Begin();
}

String::ConstIterator icinga::range_begin(const String& x)
{
	return x.Begin();
}

String::Iterator icinga::range_end(String& x)
{
	return x.End();
}

String::ConstIterator icinga::range_end(const String& x)
{
	return x.End();
}
