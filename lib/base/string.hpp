// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef STRING_H
#define STRING_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include <boost/beast/core.hpp>
#include <boost/range/iterator.hpp>
#include <boost/utility/string_view.hpp>
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

	String() = default;
	String(const char *data);
	String(std::string data);
	String(const char *data, std::size_t size);
	explicit String(std::string_view sv);
	String(String::SizeType n, char c);
	String(const String& other) = default;
	String(String&& other) noexcept;

#ifndef _MSC_VER
	String(Value&& other);
#endif /* _MSC_VER */

	template<typename InputIterator>
	String(InputIterator begin, InputIterator end)
		: m_Data(begin, end)
	{ }

	String& operator=(const String& rhs);
	String& operator=(String&& rhs) noexcept;
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

	operator std::string() const &;
	operator std::string() &&;
	operator std::string_view() const;
	operator boost::beast::string_view() const;

	const char *CStr() const;

	void Clear();

	SizeType GetLength() const;

	std::string& GetData() &;
	std::string&& GetData() &&;
	const std::string& GetData() const &;

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

	void swap(String& str) noexcept;

	Iterator erase(Iterator first, Iterator last);

	template<typename InputIterator>
	void insert(Iterator p, InputIterator first, InputIterator last)
	{
		m_Data.insert(p, first, last);
	}

	Iterator begin() { return m_Data.begin(); }
	ConstIterator begin() const { return m_Data.begin(); }
	Iterator end() { return m_Data.end(); }
	ConstIterator end() const { return m_Data.end(); }
	ReverseIterator rbegin() { return m_Data.rbegin(); }
	ConstReverseIterator rbegin() const { return m_Data.rbegin(); }
	ReverseIterator rend() { return m_Data.rend(); }
	ConstReverseIterator rend() const { return m_Data.rend(); }

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
bool operator!=(const char* lhs, const String& rhs);

String operator ""_S(const char* ptr, std::size_t size);
}

template<>
struct std::hash<icinga::String>
{
	std::size_t operator()(const icinga::String& s) const noexcept;
};

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
