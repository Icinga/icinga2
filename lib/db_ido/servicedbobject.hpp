// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SERVICEDBOBJECT_H
#define SERVICEDBOBJECT_H

#include "db_ido/dbobject.hpp"
#include "base/configobject.hpp"
#include "icinga/service.hpp"

namespace icinga
{

/**
 * A Service database object.
 *
 * @ingroup ido
 */
class ServiceDbObject final : public DbObject
{
public:
	DECLARE_PTR_TYPEDEFS(ServiceDbObject);

	ServiceDbObject(const DbType::Ptr& type, const String& name1, const String& name2);

	static void StaticInitialize();

	Dictionary::Ptr GetConfigFields() const override;
	Dictionary::Ptr GetStatusFields() const override;

	void OnConfigUpdateHeavy() override;
	void OnConfigUpdateLight() override;

	String CalculateConfigHash(const Dictionary::Ptr& configFields) const override;

private:
	void DoCommonConfigUpdate();
};

}

#endif /* SERVICEDBOBJECT_H */
