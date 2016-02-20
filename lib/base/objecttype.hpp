/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#ifndef OBJECTTYPE_H
#define OBJECTTYPE_H

#include "base/i2-base.hpp"
#include "base/type.hpp"
#include "base/initialize.hpp"

namespace icinga
{

class I2_BASE_API ObjectType : public Type
{
public:
	ObjectType(void);

	virtual String GetName(void) const override;
	virtual Type::Ptr GetBaseType(void) const override;
	virtual int GetAttributes(void) const override;
	virtual int GetFieldId(const String& name) const override;
	virtual Field GetFieldInfo(int id) const override;
	virtual int GetFieldCount(void) const override;

protected:
	virtual ObjectFactory GetFactory(void) const override;
};

}

#endif /* OBJECTTYPE_H */
