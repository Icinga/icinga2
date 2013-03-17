/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef LIVESTATUSCONNECTION_H
#define LIVESTATUSCONNECTION_H

#include "base/connection.h"

using namespace icinga;

namespace livestatus
{

class LivestatusConnection : public Connection
{
public:
	typedef shared_ptr<LivestatusConnection> Ptr;
	typedef weak_ptr<LivestatusConnection> WeakPtr;

	LivestatusConnection(const Stream::Ptr& stream);

protected:
	std::vector<String> m_Lines;

	virtual void ProcessData(void);
};

}

#endif /* LIVESTATUSCONNECTION_H */
