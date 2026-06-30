// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef OBJECTRULE_H
#define OBJECTRULE_H

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

#endif /* OBJECTRULE_H */
