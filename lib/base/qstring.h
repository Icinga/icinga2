#ifndef STRING_H
#define STRING_H

namespace icinga {

/**
 * String class.
 *
 * Rationale: The std::string class has an ambiguous assignment
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
	String SubStr(size_t first, size_t second) const;
	void Replace(size_t first, size_t second, const String& str);

	template<typename Predicate>
	vector<String> Split(const Predicate& predicate)
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
