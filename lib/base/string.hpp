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

	typedef std::string::reverse_iterator ReverseIterator;
	typedef std::string::const_reverse_iterator ConstReverseIterator;

	typedef std::string::reverse_iterator reverse_iterator;
	typedef std::string::const_reverse_iterator const_reverse_iterator;

	typedef std::string::size_type SizeType;

	inline String(void)
		: m_Data()
	{ }

	inline String(const char *data)
		: m_Data(data)
	{ }

	inline String(const std::string& data)
		: m_Data(data)
	{ }

	inline String(String::SizeType n, char c)
		: m_Data(n, c)
	{ }

	inline String(const String& other)
		: m_Data(other.m_Data)
	{ }

	inline ~String(void)
	{ }

	template<typename InputIterator>
	String(InputIterator begin, InputIterator end)
		: m_Data(begin, end)
	{ }

	inline String& operator=(const String& rhs)
	{
		m_Data = rhs.m_Data;
		return *this;
	}

	inline String& operator=(const std::string& rhs)
	{
		m_Data = rhs;
		return *this;
	}

	inline String& operator=(const char *rhs)
	{
		m_Data = rhs;
		return *this;
	}

	inline const char& operator[](SizeType pos) const
	{
		return m_Data[pos];
	}

	inline char& operator[](SizeType pos)
	{
		return m_Data[pos];
	}

	inline String& operator+=(const String& rhs)
	{
		m_Data += rhs.m_Data;
		return *this;
	}

	inline String& operator+=(const char *rhs)
	{
		m_Data += rhs;
		return *this;
	}

	String& operator+=(const Value& rhs);

	inline String& operator+=(char rhs)
	{
		m_Data += rhs;
		return *this;
	}

	inline bool IsEmpty(void) const
	{
		return m_Data.empty();
	}

	inline bool operator<(const String& rhs) const
	{
		return m_Data < rhs.m_Data;
	}

	inline operator const std::string&(void) const
	{
		return m_Data;
	}

	inline const char *CStr(void) const
	{
		return m_Data.c_str();
	}

	inline void Clear(void)
	{
		m_Data.clear();
	}

	inline SizeType GetLength(void) const
	{
		return m_Data.size();
	}

	inline std::string& GetData(void)
	{
		return m_Data;
	}

	inline const std::string& GetData(void) const
	{
		return m_Data;
	}

	inline SizeType Find(const String& str, SizeType pos = 0) const
	{
		return m_Data.find(str, pos);
	}

	inline SizeType RFind(const String& str, SizeType pos = NPos) const
	{
		return m_Data.rfind(str, pos);
	}

	inline SizeType FindFirstOf(const char *s, SizeType pos = 0) const
	{
		return m_Data.find_first_of(s, pos);
	}

	inline SizeType FindFirstOf(char ch, SizeType pos = 0) const
	{
		return m_Data.find_first_of(ch, pos);
	}

	inline SizeType FindFirstNotOf(const char *s, SizeType pos = 0) const
	{
		return m_Data.find_first_not_of(s, pos);
	}

	inline SizeType FindFirstNotOf(char ch, SizeType pos = 0) const
	{
		return m_Data.find_first_not_of(ch, pos);
	}

	inline String SubStr(SizeType first, SizeType len = NPos) const
	{
		return m_Data.substr(first, len);
	}

	inline void Replace(SizeType first, SizeType second, const String& str)
	{
		m_Data.replace(first, second, str);
	}

	void Trim(void);

	inline bool Contains(const String& str) const
	{
		return (m_Data.find(str) != std::string::npos);
	}

	inline void swap(String& str)
	{
		m_Data.swap(str.m_Data);
	}

	inline Iterator erase(Iterator first, Iterator last)
	{
		return m_Data.erase(first, last);
	}

	template<typename InputIterator>
	void insert(Iterator p, InputIterator first, InputIterator last)
	{
		m_Data.insert(p, first, last);
	}

	inline Iterator Begin(void)
	{
		return m_Data.begin();
	}

	inline ConstIterator Begin(void) const
	{
		return m_Data.begin();
	}

	inline Iterator End(void)
	{
		return m_Data.end();
	}

	inline ConstIterator End(void) const
	{
		return m_Data.end();
	}

	inline ReverseIterator RBegin(void)
	{
		return m_Data.rbegin();
	}

	inline ConstReverseIterator RBegin(void) const
	{
		return m_Data.rbegin();
	}

	inline ReverseIterator REnd(void)
	{
		return m_Data.rend();
	}

	inline ConstReverseIterator REnd(void) const
	{
		return m_Data.rend();
	}

	static const SizeType NPos;

	static Object::Ptr GetPrototype(void);

private:
	std::string m_Data;
};

inline std::ostream& operator<<(std::ostream& stream, const String& str)
{
	stream << static_cast<std::string>(str);
	return stream;
}

inline std::istream& operator>>(std::istream& stream, String& str)
{
	std::string tstr;
	stream >> tstr;
	str = tstr;
	return stream;
}

inline String operator+(const String& lhs, const String& rhs)
{
	return static_cast<std::string>(lhs)+static_cast<std::string>(rhs);
}

inline String operator+(const String& lhs, const char *rhs)
{
	return static_cast<std::string>(lhs)+rhs;
}

inline String operator+(const char *lhs, const String& rhs)
{
	return lhs + static_cast<std::string>(rhs);
}

inline bool operator==(const String& lhs, const String& rhs)
{
	return static_cast<std::string>(lhs) == static_cast<std::string>(rhs);
}

inline bool operator==(const String& lhs, const char *rhs)
{
	return static_cast<std::string>(lhs) == rhs;
}

inline bool operator==(const char *lhs, const String& rhs)
{
	return lhs == static_cast<std::string>(rhs);
}

inline bool operator<(const String& lhs, const char *rhs)
{
	return static_cast<std::string>(lhs) < rhs;
}

inline bool operator<(const char *lhs, const String& rhs)
{
	return lhs < static_cast<std::string>(rhs);
}

inline bool operator>(const String& lhs, const String& rhs)
{
	return static_cast<std::string>(lhs) > static_cast<std::string>(rhs);
}

inline bool operator>(const String& lhs, const char *rhs)
{
	return static_cast<std::string>(lhs) > rhs;
}

inline bool operator>(const char *lhs, const String& rhs)
{
	return lhs > static_cast<std::string>(rhs);
}

inline bool operator<=(const String& lhs, const String& rhs)
{
	return static_cast<std::string>(lhs) <= static_cast<std::string>(rhs);
}

inline bool operator<=(const String& lhs, const char *rhs)
{
	return static_cast<std::string>(lhs) <= rhs;
}

inline bool operator<=(const char *lhs, const String& rhs)
{
	return lhs <= static_cast<std::string>(rhs);
}

inline bool operator>=(const String& lhs, const String& rhs)
{
	return static_cast<std::string>(lhs) >= static_cast<std::string>(rhs);
}

inline bool operator>=(const String& lhs, const char *rhs)
{
	return static_cast<std::string>(lhs) >= rhs;
}

inline bool operator>=(const char *lhs, const String& rhs)
{
	return lhs >= static_cast<std::string>(rhs);
}

inline bool operator!=(const String& lhs, const String& rhs)
{
	return static_cast<std::string>(lhs) != static_cast<std::string>(rhs);
}

inline bool operator!=(const String& lhs, const char *rhs)
{
	return static_cast<std::string>(lhs) != rhs;
}

inline bool operator!=(const char *lhs, const String& rhs)
{
	return lhs != static_cast<std::string>(rhs);
}

inline String::Iterator range_begin(String& x)
{
	return x.Begin();
}

inline String::ConstIterator range_begin(const String& x)
{
	return x.Begin();
}

inline String::Iterator range_end(String& x)
{
	return x.End();
}

inline String::ConstIterator range_end(const String& x)
{
	return x.End();
}

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
