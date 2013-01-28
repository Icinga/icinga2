/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

namespace icinga {

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

	String(void);
	String(const char *data);
	String(const std::string& data);

	template<typename InputIterator>
	String(InputIterator begin, InputIterator end)
		: m_Data(begin, end)
	{ }

	String(const String& other);

	String& operator=(const String& rhs);
	String& operator=(const std::string& rhs);
	String& operator=(const char *rhs);

	const char& operator[](size_t pos) const;
	char& operator[](size_t pos);

	String& operator+=(const String& rhs);
	String& operator+=(const char *rhs);

	bool IsEmpty(void) const;

	bool operator<(const String& rhs) const;

	operator const std::string&(void) const;

	const char *CStr(void) const;
	void Clear(void);
	size_t GetLength(void) const;

	size_t FindFirstOf(const char *s, size_t pos = 0) const;
	String SubStr(size_t first, size_t len) const;
	void Replace(size_t first, size_t second, const String& str);

	template<typename Predicate>
	vector<String> Split(const Predicate& predicate) const
	{
		vector<String> tokens;
		boost::algorithm::split(tokens, m_Data, predicate);
		return tokens;
	}

	void Trim(void);

	void swap(String& str);
	Iterator erase(Iterator first, Iterator last);

	Iterator Begin(void);
	ConstIterator Begin(void) const;
	Iterator End(void);
	ConstIterator End(void) const;

	double ToDouble(void) const;

	static const size_t NPos;

private:
	std::string m_Data;
};

I2_BASE_API ostream& operator<<(ostream& stream, const String& str);
I2_BASE_API istream& operator>>(istream& stream, String& str);

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
I2_BASE_API bool operator>(const String& lhs, const char *rhs);
I2_BASE_API bool operator>(const char *lhs, const String& rhs);

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
