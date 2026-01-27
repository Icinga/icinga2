// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
