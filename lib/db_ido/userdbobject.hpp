/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef USERDBOBJECT_H
#define USERDBOBJECT_H

#include "db_ido/dbobject.hpp"
#include "base/configobject.hpp"

namespace icinga
{

/**
 * A User database object.
 *
 * @ingroup ido
 */
class UserDbObject final : public DbObject
{
public:
	DECLARE_PTR_TYPEDEFS(UserDbObject);

	UserDbObject(const DbType::Ptr& type, const String& name1, const String& name2);

protected:
	Dictionary::Ptr GetConfigFields() const override;
	Dictionary::Ptr GetStatusFields() const override;

	void OnConfigUpdateHeavy(std::vector<DbQuery>& queries) override;

	String CalculateConfigHash(const Dictionary::Ptr& configFields) const override;
};

}

#endif /* USERDBOBJECT_H */
