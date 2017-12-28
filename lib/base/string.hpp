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

#pragma once

#ifndef STRING_H
#define STRING_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include <boost/range/iterator.hpp>
#include <string.h>
#include <functional>
#include <string>
#include <iosfwd>

namespace icinga {

/**
 * String class.
 *
 * Rationale for having this: The std::string class has an ambiguous assignment
 * operator when used in conjunction with the Value class.
 */
class String
{
public:
	typedef std::string::const_iterator ConstIterator;
	typedef std::string::const_iterator const_iterator;
	typedef std::string::const_reverse_iterator ConstReverseIterator;
	typedef std::string::const_reverse_iterator const_reverse_iterator;

	typedef std::string::size_type SizeType;

	String(void);
	String(const char *data);
	String(const std::string& data);
	String(std::string&& data);
	String(String::SizeType n, char c);
	String(const String& other);

	~String(void);

	String& operator=(const String& other);

	template<typename InputIterator>
	String(InputIterator begin, InputIterator end)
		: m_Data(begin, end)
	{ }

	const char& operator[](SizeType pos) const;

	bool IsEmpty(void) const;

	bool operator<(const String& rhs) const;

	operator const std::string&(void) const;
	explicit operator double(void) const;
	explicit operator int(void) const;

	const char *CStr(void) const;

	SizeType GetLength(void) const;

	const std::string& GetData(void) const;

	SizeType Find(const String& str, SizeType pos = 0) const;
	SizeType RFind(const String& str, SizeType pos = NPos) const;

	SizeType FindFirstOf(const char *s, SizeType pos = 0) const;
	SizeType FindFirstOf(char ch, SizeType pos = 0) const;
	SizeType FindFirstNotOf(const char *s, SizeType pos = 0) const;
	SizeType FindFirstNotOf(char ch, SizeType pos = 0) const;
	SizeType FindLastOf(const char *s, SizeType pos = NPos) const;
	SizeType FindLastOf(char ch, SizeType pos = NPos) const;

	String SubStr(SizeType first, SizeType len = NPos) const;

	std::vector<String> Split(const char *separators) const;

	String ReplaceAll(const String& srch, const String& rep) const;
	String ReplaceAll(const std::initializer_list<String>& srch, const std::initializer_list<String>& rep) const;

	String Trim(void) const;

	String ToLower(void) const;

	String ToUpper(void) const;

	String Reverse(void) const;

	bool Contains(const String& str) const;

	ConstIterator Begin(void) const;
	ConstIterator End(void) const;

	ConstReverseIterator RBegin(void) const;
	ConstReverseIterator REnd(void) const;

	static const SizeType NPos;

	static Object::Ptr GetPrototype(void);

private:
	std::string m_Data;
};

std::ostream& operator<<(std::ostream& stream, const String& str);
std::istream& operator>>(std::istream& stream, String& str);

String operator+(const String& lhs, const String& rhs);
String operator+(const String& lhs, const char *rhs);
String operator+(const char *lhs, const String& rhs);

bool operator==(const String& lhs, const String& rhs);
bool operator==(const String& lhs, const char *rhs);
bool operator==(const char *lhs, const String& rhs);

bool operator<(const String& lhs, const char *rhs);
bool operator<(const char *lhs, const String& rhs);

bool operator>(const String& lhs, const String& rhs);
bool operator>(const String& lhs, const char *rhs);
bool operator>(const char *lhs, const String& rhs);

bool operator<=(const String& lhs, const String& rhs);
bool operator<=(const String& lhs, const char *rhs);
bool operator<=(const char *lhs, const String& rhs);

bool operator>=(const String& lhs, const String& rhs);
bool operator>=(const String& lhs, const char *rhs);
bool operator>=(const char *lhs, const String& rhs);

bool operator!=(const String& lhs, const String& rhs);
bool operator!=(const String& lhs, const char *rhs);
bool operator!=(const char *lhs, const String& rhs);

String::ConstIterator begin(const String& x);
String::ConstIterator end(const String& x);
String::ConstIterator range_begin(const String& x);
String::ConstIterator range_end(const String& x);

}

extern template class std::vector<icinga::String>;

namespace boost
{

template<>
struct range_const_iterator<icinga::String>
{
	typedef icinga::String::ConstIterator type;
};

}

#endif /* STRING_H */
