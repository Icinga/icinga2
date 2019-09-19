/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef STRINGBUILDER_H
#define STRINGBUILDER_H

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include <string>
#include <vector>

namespace icinga
{

/**
 * A string builder.
 *
 * @ingroup base
 */
class StringBuilder final
{
public:
	void Append(const String&);
	void Append(const std::string&);
	void Append(const char *, const char *);
	void Append(const char *);
	void Append(char);

	String ToString() const;

private:
	std::vector<char> m_Buffer;
};

}

#endif /* STRINGBUILDER_H */
