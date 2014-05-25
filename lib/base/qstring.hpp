/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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
#include <string.h>
#include <functional>
#include <string>
#include <istream>

namespace icinga {

class Value;

/**
 * String class.
 *
 * Rationale for having this: The std::string class has an ambiguous assignment
 * operator when used in conjunction with the Value class.
 */
class I2_BASE_API String
{
public:
	typedef std::string::iterator Iterator;
	typedef std::string::const_iterator ConstIterator;

	typedef std::string::iterator iterator;
	typedef std::string::const_iterator const_iterator;

	typedef std::string::size_type SizeType;

	String(void);
	String(const char *data);
	String(const std::string& data);
	String(SizeType n, char c);

	template<typename InputIterator>
	String(InputIterator begin, InputIterator end)
		: m_Data(begin, end)
	{ }

	String(const String& other);

	String& operator=(const String& rhs);
	String& operator=(const std::string& rhs);
	String& operator=(const char *rhs);

	const char& operator[](SizeType pos) const;
	char& operator[](SizeType pos);

	String& operator+=(const String& rhs);
	String& operator+=(const char *rhs);
	String& operator+=(const Value& rhs);
	String& operator+=(char rhs);

	bool IsEmpty(void) const;

	bool operator<(const String& rhs) const;

	operator const std::string&(void) const;

	const char *CStr(void) const;
	void Clear(void);
	SizeType GetLength(void) const;

	std::string& GetData(void);
	const std::string& GetData(void) const;

	SizeType Find(const String& str, SizeType pos = 0) const;
	SizeType RFind(const String& str, SizeType pos = NPos) const;
	SizeType FindFirstOf(const char *s, SizeType pos = 0) const;
	SizeType FindFirstOf(char ch, SizeType pos = 0) const;
	SizeType FindFirstNotOf(const char *s, SizeType pos = 0) const;
	SizeType FindFirstNotOf(char ch, SizeType pos = 0) const;
	String SubStr(SizeType first, SizeType len = NPos) const;
	void Replace(SizeType first, SizeType second, const String& str);

	void Trim(void);
	bool Contains(const String& str) const;

	void swap(String& str);
	Iterator erase(Iterator first, Iterator last);

	template<typename InputIterator>
	void insert(Iterator p, InputIterator first, InputIterator last)
	{
		m_Data.insert(p, first, last);
	}

	Iterator Begin(void);
	ConstIterator Begin(void) const;
	Iterator End(void);
	ConstIterator End(void) const;

	static const SizeType NPos;

private:
	std::string m_Data;
};

I2_BASE_API std::ostream& operator<<(std::ostream& stream, const String& str);
I2_BASE_API std::istream& operator>>(std::istream& stream, String& str);

I2_BASE_API String operator+(const String& lhs, const String& rhs);
I2_BASE_API String operator+(const String& lhs, const char *rhs);
I2_BASE_API String operator+(const char *lhs, const String& rhs);

I2_BASE_API bool operator==(const String& lhs, const String& rhs);
I2_BASE_API bool operator==(const String& lhs, const char *rhs);
I2_BASE_API bool operator==(const char *lhs, const String& rhs);

I2_BASE_API bool operator!=(const String& lhs, const String& rhs);
I2_BASE_API bool operator!=(const String& lhs, const char *rhs);
I2_BASE_API bool operator!=(const char *lhs, const String& rhs);

I2_BASE_API bool operator<(const String& lhs, const char *rhs);
I2_BASE_API bool operator<(const char *lhs, const String& rhs);

I2_BASE_API bool operator>(const String& lhs, const String& rhs);
I2_BASE_API bool operator>(const String& lhs, const char *rhs);
I2_BASE_API bool operator>(const char *lhs, const String& rhs);

I2_BASE_API bool operator<=(const String& lhs, const String& rhs);
I2_BASE_API bool operator<=(const String& lhs, const char *rhs);
I2_BASE_API bool operator<=(const char *lhs, const String& rhs);

I2_BASE_API bool operator>=(const String& lhs, const String& rhs);
I2_BASE_API bool operator>=(const String& lhs, const char *rhs);
I2_BASE_API bool operator>=(const char *lhs, const String& rhs);

I2_BASE_API String::Iterator range_begin(String& x);
I2_BASE_API String::ConstIterator range_begin(const String& x);
I2_BASE_API String::Iterator range_end(String& x);
I2_BASE_API String::ConstIterator range_end(const String& x);

struct string_iless : std::binary_function<String, String, bool>
{
	bool operator()(const String& s1, const String& s2) const
	{
		 return strcasecmp(s1.CStr(), s2.CStr()) < 0;

		 /* The "right" way would be to do this - however the
		  * overhead is _massive_ due to the repeated non-inlined
		  * function calls:

		 return lexicographical_compare(s1.Begin(), s1.End(),
		     s2.Begin(), s2.End(), boost::algorithm::is_iless());

		  */
	}
};

}

namespace boost
{

template <typename C> struct range_mutable_iterator;

template<>
struct range_mutable_iterator<icinga::String>
{
	typedef icinga::String::Iterator type;
};

template <typename C> struct range_const_iterator;

template<>
struct range_const_iterator<icinga::String>
{
	typedef icinga::String::ConstIterator type;
};

}

#endif /* STRING_H */
