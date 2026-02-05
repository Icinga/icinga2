// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "remote/messageorigin.hpp"

using namespace icinga;

bool MessageOrigin::IsLocal() const
{
	return !FromClient;
}
