// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TIMEPERIODDBOBJECT_H
#define TIMEPERIODDBOBJECT_H

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

#endif /* TIMEPERIODDBOBJECT_H */
