/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "db_ido/dbobject.hpp"
#include "icinga/hostgroup.hpp"
#include "base/configobject.hpp"

namespace icinga
{

/**
 * A HostGroup database object.
 *
 * @ingroup ido
 */
class HostGroupDbObject final : public DbObject
{
public:
	DECLARE_PTR_TYPEDEFS(HostGroupDbObject);

	HostGroupDbObject(const DbType::Ptr& type, const String& name1, const String& name2);

	Dictionary::Ptr GetConfigFields() const override;
	Dictionary::Ptr GetStatusFields() const override;

private:
	static void MembersChangedHandler(const HostGroup::Ptr& hgfilter);
};

}
