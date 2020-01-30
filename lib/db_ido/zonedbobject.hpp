/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "db_ido/dbobject.hpp"
#include "base/configobject.hpp"
#include "remote/zone.hpp"

namespace icinga
{

/**
 * An Endpoint database object.
 *
 * @ingroup ido
 */
class ZoneDbObject final : public DbObject
{
public:
	DECLARE_PTR_TYPEDEFS(ZoneDbObject);

	ZoneDbObject(const intrusive_ptr<DbType>& type, const String& name1, const String& name2);

	Dictionary::Ptr GetConfigFields() const override;
	Dictionary::Ptr GetStatusFields() const override;
};

}
