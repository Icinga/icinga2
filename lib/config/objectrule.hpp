/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "config/i2-config.hpp"
#include "config/expression.hpp"
#include "base/debuginfo.hpp"
#include <set>

namespace icinga
{

/**
 * @ingroup config
 */
class ObjectRule
{
public:
	typedef std::set<String> TypeSet;

	static void RegisterType(const String& sourceType);
	static bool IsValidSourceType(const String& sourceType);

private:
	ObjectRule();

	static TypeSet m_Types;
};

}
