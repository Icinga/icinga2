/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

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
