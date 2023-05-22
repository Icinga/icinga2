/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

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
