// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef OBJECTUTILS_H
#define OBJECTUTILS_H

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include "base/array.hpp"
#include "icinga/service.hpp"

namespace icinga
{

/**
 * @ingroup icinga
 */
class ObjectUtils
{
public:
	static Service::Ptr GetService(const Value& host, const String& name);
	static Array::Ptr GetServices(const Value& host);

private:
	ObjectUtils();
};

}

#endif /* OBJECTUTILS_H */
