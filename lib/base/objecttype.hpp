/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef OBJECTTYPE_H
#define OBJECTTYPE_H

#include "base/i2-base.hpp"
#include "base/initialize.hpp"
#include "base/type.hpp"

namespace icinga
{

class ObjectType final : public Type
{
public:
	String GetName() const override;
	Type::Ptr GetBaseType() const override;
	int GetAttributes() const override;
	int GetFieldId(const String& name) const override;
	Field GetFieldInfo(int id) const override;
	int GetFieldCount() const override;

protected:
	ObjectFactory GetFactory() const override;
};

}

#endif /* OBJECTTYPE_H */
