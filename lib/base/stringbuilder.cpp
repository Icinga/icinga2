/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/stringbuilder.hpp"
#include <cstring>

using namespace icinga;

void StringBuilder::Append(const String& str)
{
	m_Buffer.insert(m_Buffer.end(), str.Begin(), str.End());
}

void StringBuilder::Append(const std::string& str)
{
	m_Buffer.insert(m_Buffer.end(), str.begin(), str.end());
}

void StringBuilder::Append(const char *begin, const char *end)
{
	m_Buffer.insert(m_Buffer.end(), begin, end);
}

void StringBuilder::Append(const char *cstr)
{
	m_Buffer.insert(m_Buffer.end(), cstr, cstr + std::strlen(cstr));
}

void StringBuilder::Append(char chr)
{
	m_Buffer.emplace_back(chr);
}

String StringBuilder::ToString() const
{
	return String(m_Buffer.data(), m_Buffer.data() + m_Buffer.size());
}
