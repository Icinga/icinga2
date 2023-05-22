/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "db_ido/dbobject.hpp"
#include "icinga/servicegroup.hpp"
#include "base/configobject.hpp"

namespace icinga
{

/**
 * A ServiceGroup database object.
 *
 * @ingroup ido
 */
class ServiceGroupDbObject final : public DbObject
{
public:
	DECLARE_PTR_TYPEDEFS(ServiceGroupDbObject);

	ServiceGroupDbObject(const DbType::Ptr& type, const String& name1, const String& name2);

	Dictionary::Ptr GetConfigFields() const override;
	Dictionary::Ptr GetStatusFields() const override;
};

}
