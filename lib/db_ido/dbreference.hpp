// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DBREFERENCE_H
#define DBREFERENCE_H

#include "db_ido/i2-db_ido.hpp"

namespace icinga
{

/**
 * A database reference.
 *
 * @ingroup ido
 */
struct DbReference
{
public:
	DbReference() = default;
	DbReference(long id);

	bool IsValid() const;
	operator long() const;
private:
	long m_Id{-1};
};

}

#endif /* DBREFERENCE_H */
