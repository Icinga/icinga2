/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "base/i2-base.hpp"
#include "base/type.hpp"
#include "base/value.hpp"

namespace icinga
{

I2_BASE_API Value Serialize(const Value& value, int attributeTypes = FAState);
I2_BASE_API Value Deserialize(const Value& value, bool safe_mode = false, int attributeTypes = FAState);
I2_BASE_API Value Deserialize(const Object::Ptr& object, const Value& value, bool safe_mode = false, int attributeTypes = FAState);

}

#endif /* SERIALIZER_H */
