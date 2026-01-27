// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dbreference.hpp"

using namespace icinga;

DbReference::DbReference(long id)
	: m_Id(id)
{ }

bool DbReference::IsValid() const
{
	return (m_Id != -1);
}

DbReference::operator long() const
{
	return m_Id;
}
