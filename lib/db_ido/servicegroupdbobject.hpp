// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SERVICEGROUPDBOBJECT_H
#define SERVICEGROUPDBOBJECT_H

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

#endif /* SERVICEGROUPDBOBJECT_H */
