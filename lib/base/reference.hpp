/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#ifndef REFERENCE_H
#define REFERENCE_H

#include "base/i2-base.hpp"
#include "base/objectlock.hpp"
#include "base/value.hpp"

namespace icinga
{

/**
 * A reference.
 *
 * @ingroup base
 */
class Reference final : public Object
{
public:
	DECLARE_OBJECT(Reference);

	Reference(const Object::Ptr& parent, const String& index);

	Value Get() const;
	void Set(const Value& value);

	Object::Ptr GetParent() const;
	String GetIndex() const;

	static Object::Ptr GetPrototype();

private:
	Object::Ptr m_Parent;
	String m_Index;
};

}

#endif /* REFERENCE_H */
