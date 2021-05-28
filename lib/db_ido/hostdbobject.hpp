/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef HOSTDBOBJECT_H
#define HOSTDBOBJECT_H

#include "db_ido/dbobject.hpp"
#include "base/configobject.hpp"

namespace icinga
{

/**
 * A Host database object.
 *
 * @ingroup ido
 */
class HostDbObject final : public DbObject
{
public:
	DECLARE_PTR_TYPEDEFS(HostDbObject);

	HostDbObject(const DbType::Ptr& type, const String& name1, const String& name2);

	Dictionary::Ptr GetConfigFields() const override;
	Dictionary::Ptr GetStatusFields() const override;

	void OnConfigUpdateHeavy(std::vector<DbQuery>& queries) override;
	void OnConfigUpdateLight() override;

	String CalculateConfigHash(const Dictionary::Ptr& configFields) const override;

private:
	void DoCommonConfigUpdate();
};

}

#endif /* HOSTDBOBJECT_H */
