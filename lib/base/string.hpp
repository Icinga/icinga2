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

#ifndef STRING_H
#define STRING_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/iterator.hpp>
#include <string.h>
#include <functional>
#include <string>
#include <iosfwd>

namespace icinga {

class Value;

/**
 * String class.
 *
 * Rationale for having this: The std::string class has an ambiguous assignment
 * operator when used in conjunction with the Value class.
 */
class String
{
public:
	typedef std::string::iterator Iterator;
	typedef std::string::const_iterator ConstIterator;

	typedef std::string::iterator iterator;
	typedef std::string::const_iterator const_iterator;

	typedef std::string::reverse_iterator ReverseIterator;
	typedef std::string::const_reverse_iterator ConstReverseIterator;

	typedef std::string::reverse_iterator reverse_iterator;
	typedef std::string::const_reverse_iterator const_reverse_iterator;

	typedef std::string::size_type SizeType;

	String();
	String(const char *data);
	String(std::string data);
	String(String::SizeType n, char c);
	String(const String& other);
	String(String&& other);

#ifndef _MSC_VER
	String(Value&& other);
#endif /* _MSC_VER */

	~String();

	template<typename InputIterator>
	String(InputIterator begin, InputIterator end)
		: m_Data(begin, end)
	{ }

	String& operator=(const String& rhs);
	String& operator=(String&& rhs);
	String& operator=(Value&& rhs);
	String& operator=(const std::string& rhs);
	String& operator=(const char *rhs);

	const char& operator[](SizeType pos) const;
	char& operator[](SizeType pos);

	String& operator+=(const String& rhs);
	String& operator+=(const char *rhs);
	String& operator+=(const Value& rhs);
	String& operator+=(char rhs);

	bool IsEmpty() const;

	bool operator<(const String& rhs) const;

	operator const std::string&() const;

	const char *CStr() const;

	void Clear();

	SizeType GetLength() const;

	std::string& GetData();
	const std::string& GetData() const;

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

	void Replace(SizeType first, SizeType second, const String& str);

	String Trim() const;

	String ToLower() const;

	String ToUpper() const;

	String Reverse() const;

	void Append(int count, char ch);

	bool Contains(const String& str) const;

	void swap(String& str);

	Iterator erase(Iterator first, Iterator last);

	template<typename InputIterator>
	void insert(Iterator p, InputIterator first, InputIterator last)
	{
		m_Data.insert(p, first, last);
	}

	Iterator Begin();
	ConstIterator Begin() const;
	Iterator End();
	ConstIterator End() const;
	ReverseIterator RBegin();
	ConstReverseIterator RBegin() const;
	ReverseIterator REnd();
	ConstReverseIterator REnd() const;

	static const SizeType NPos;

	static Object::Ptr GetPrototype();

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

String::Iterator begin(String& x);
String::ConstIterator begin(const String& x);
String::Iterator end(String& x);
String::ConstIterator end(const String& x);
String::Iterator range_begin(String& x);
String::ConstIterator range_begin(const String& x);
String::Iterator range_end(String& x);
String::ConstIterator range_end(const String& x);

}

extern template class std::vector<icinga::String>;

namespace boost
{

template<>
struct range_mutable_iterator<icinga::String>
{
	typedef icinga::String::Iterator type;
};

template<>
struct range_const_iterator<icinga::String>
{
	typedef icinga::String::ConstIterator type;
};

}

#endif /* STRING_H */
