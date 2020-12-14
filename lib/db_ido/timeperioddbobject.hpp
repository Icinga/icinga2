/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "db_ido/dbobject.hpp"
#include "base/configobject.hpp"

namespace icinga
{

/**
 * A TimePeriod database object.
 *
 * @ingroup ido
 */
class TimePeriodDbObject final : public DbObject
{
public:
	DECLARE_PTR_TYPEDEFS(TimePeriodDbObject);

	TimePeriodDbObject(const DbType::Ptr& type, const String& name1, const String& name2);

protected:
	Dictionary::Ptr GetConfigFields() const override;
	Dictionary::Ptr GetStatusFields() const override;

	void OnConfigUpdateHeavy() override;
};

}
