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

#include <cstdio>
#include <iostream>
#include "i2-icinga.h"

#ifndef _WIN32
#	include "icinga-version.h"
#	define ICINGA_VERSION GIT_MESSAGE
#endif /* _WIN32 */

using namespace icinga;

/**
 * The entry point for the Icinga application.
 *
 * @param args Command-line arguments.
 * @returns An exit status.
 */
int IcingaApplication::Main(const vector<string>& args)
{
#ifdef _WIN32
	Application::Log("Icinga component loader");
#else /* _WIN32 */
	Application::Log("Icinga component loader (version: " ICINGA_VERSION ")");
#endif  /* _WIN32 */

	if (args.size() < 2) {
		cout << "Syntax: " << args[0] << " <config-file>" << endl;
		return EXIT_FAILURE;
	}

	m_EndpointManager = make_shared<EndpointManager>();

	string componentDirectory = GetExeDirectory() + "/../lib/icinga2";
	AddComponentSearchDir(componentDirectory);

	/* register handler for 'component' config objects */
	static ConfigObject::Set::Ptr componentObjects = make_shared<ConfigObject::Set>(ConfigObject::GetAllObjects(), ConfigObject::MakeTypePredicate("component"));
	function<int (const ObjectSetEventArgs<ConfigObject::Ptr>&)> NewComponentHandler = bind(&IcingaApplication::NewComponentHandler, this, _1);
	componentObjects->OnObjectAdded.connect(NewComponentHandler);
	componentObjects->OnObjectCommitted.connect(NewComponentHandler);
	componentObjects->OnObjectRemoved.connect(bind(&IcingaApplication::DeletedComponentHandler, this, _1));
	componentObjects->Start();

	/* load config file */
	ConfigObject::Ptr fileComponentConfig = make_shared<ConfigObject>("component", "configfile");
	fileComponentConfig->SetLocal(true);
	fileComponentConfig->SetProperty("configFilename", args[1]);
	fileComponentConfig->Commit();

	ConfigObject::Ptr icingaConfig = ConfigObject::GetObject("application", "icinga");

	if (!icingaConfig)
		throw runtime_error("Configuration must contain an 'application' object named 'icinga'.");

	if (!icingaConfig->IsLocal())
		throw runtime_error("'icinga' application object must be 'local'.");

	icingaConfig->GetProperty("privkey", &m_PrivateKeyFile);
	icingaConfig->GetProperty("pubkey", &m_PublicKeyFile);
	icingaConfig->GetProperty("cakey", &m_CAKeyFile);
	icingaConfig->GetProperty("node", &m_Node);
	icingaConfig->GetProperty("service", &m_Service);

	if (!GetPrivateKeyFile().empty() && !GetPublicKeyFile().empty() && !GetCAKeyFile().empty()) {
		/* set up SSL context */
		shared_ptr<X509> cert = Utility::GetX509Certificate(GetPublicKeyFile());
		string identity = Utility::GetCertificateCN(cert);
		Application::Log("My identity: " + identity);
		m_EndpointManager->SetIdentity(identity);

		shared_ptr<SSL_CTX> sslContext = Utility::MakeSSLContext(GetPublicKeyFile(), GetPrivateKeyFile(), GetCAKeyFile());
		m_EndpointManager->SetSSLContext(sslContext);
	}

	/* create the primary RPC listener */
	string service = GetService();
	if (!service.empty())
		GetEndpointManager()->AddListener(service);

	RunEventLoop();

	return EXIT_SUCCESS;
}

/**
 * Retrieves Icinga's endpoint manager.
 *
 * @returns The endpoint manager.
 */
EndpointManager::Ptr IcingaApplication::GetEndpointManager(void)
{
	return m_EndpointManager;
}

int IcingaApplication::NewComponentHandler(const ObjectSetEventArgs<ConfigObject::Ptr>& ea)
{
	ConfigObject::Ptr object = ea.Target;
	
	/* don't allow replicated config objects */
	if (!object->IsLocal())
		throw runtime_error("'component' objects must be 'local'");

	string path;
	if (!object->GetProperty("path", &path)) {
#ifdef _WIN32
		path = object->GetName() + ".dll";
#else /* _WIN32 */
		path = object->GetName() + ".la";
#endif /* _WIN32 */
	}

	LoadComponent(path, object);

	return 0;
}

int IcingaApplication::DeletedComponentHandler(const ObjectSetEventArgs<ConfigObject::Ptr>& ea)
{
	ConfigObject::Ptr object = ea.Target;

	Component::Ptr component = GetComponent(object->GetName());
	UnregisterComponent(component);

	return 0;
}

string IcingaApplication::GetPrivateKeyFile(void) const
{
	return m_PrivateKeyFile;
}

string IcingaApplication::GetPublicKeyFile(void) const
{
	return m_PublicKeyFile;
}

string IcingaApplication::GetCAKeyFile(void) const
{
	return m_CAKeyFile;
}

string IcingaApplication::GetNode(void) const
{
	return m_Node;
}

string IcingaApplication::GetService(void) const
{
	return m_Service;
}
