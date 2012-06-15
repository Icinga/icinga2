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

#ifndef ICINGAAPPLICATION_H
#define ICINGAAPPLICATION_H

namespace icinga
{

/**
 * The Icinga application.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API IcingaApplication : public Application
{
public:
	typedef shared_ptr<IcingaApplication> Ptr;
	typedef weak_ptr<IcingaApplication> WeakPtr;

	int Main(const vector<string>& args);

	EndpointManager::Ptr GetEndpointManager(void);

	string GetPrivateKeyFile(void) const;
	string GetPublicKeyFile(void) const;
	string GetCAKeyFile(void) const;
	string GetNode(void) const;
	string GetService(void) const;

private:
	EndpointManager::Ptr m_EndpointManager;

	string m_PrivateKeyFile;
	string m_PublicKeyFile;
	string m_CAKeyFile;
	string m_Node;
	string m_Service;

	void NewComponentHandler(const ObjectSetEventArgs<ConfigObject::Ptr>& ea);
	void DeletedComponentHandler(const ObjectSetEventArgs<ConfigObject::Ptr>& ea);
};

}

#endif /* ICINGAAPPLICATION_H */
