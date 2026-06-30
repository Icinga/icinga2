// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CUSTOMVAROBJECT_H
#define CUSTOMVAROBJECT_H

#include "icinga/i2-icinga.hpp"
#include "icinga/customvarobject-ti.hpp"
#include "base/configobject.hpp"
#include "remote/messageorigin.hpp"

namespace icinga
{

/**
 * An object with custom variable attribute.
 *
 * @ingroup icinga
 */
class CustomVarObject : public ObjectImpl<CustomVarObject>
{
public:
	DECLARE_OBJECT(CustomVarObject);

	void ValidateVars(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils) final;
};

int FilterArrayToInt(const Array::Ptr& typeFilters, const std::map<String, int>& filterMap, int defaultValue);

}

#endif /* CUSTOMVAROBJECT_H */
