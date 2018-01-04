/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef CUSTOMVAROBJECT_H
#define CUSTOMVAROBJECT_H

#include "icinga/i2-icinga.hpp"
#include "icinga/customvarobject.thpp"
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

	virtual void ValidateVars(const Dictionary::Ptr& value, const ValidationUtils& utils) override final;
};

int FilterArrayToInt(const Array::Ptr& typeFilters, const std::map<String, int>& filterMap, int defaultValue);

}

#endif /* CUSTOMVAROBJECT_H */
