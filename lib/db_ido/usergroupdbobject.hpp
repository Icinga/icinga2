/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef USERGROUPDBOBJECT_H
#define USERGROUPDBOBJECT_H

#include "db_ido/dbobject.hpp"
#include "icinga/usergroup.hpp"
#include "base/configobject.hpp"

namespace icinga
{

/**
 * A UserGroup database object.
 *
 * @ingroup ido
 */
class UserGroupDbObject final : public DbObject
{
public:
	DECLARE_PTR_TYPEDEFS(UserGroupDbObject);

	UserGroupDbObject(const DbType::Ptr& type, const String& name1, const String& name2);

	Dictionary::Ptr GetConfigFields() const override;
	Dictionary::Ptr GetStatusFields() const override;
};

}

#endif /* USERGROUPDBOBJECT_H */
