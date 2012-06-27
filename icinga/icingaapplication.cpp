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
	Application::Log(LogInformation, "icinga", "Icinga component loader");
#else /* _WIN32 */
	Application::Log(LogInformation, "icinga", "Icinga component loader (version: " ICINGA_VERSION ")");
#endif  /* _WIN32 */

	if (args.size() < 2) {
		stringstream msgbuf;
		msgbuf << "Syntax: " << args[0] << " <config-file>";
		Application::Log(LogInformation, "icinga", msgbuf.str());
		return EXIT_FAILURE;
	}

	m_EndpointManager = boost::make_shared<EndpointManager>();

	string componentDirectory = GetExeDirectory() + "/../lib/icinga2";
	AddComponentSearchDir(componentDirectory);

	/* register handler for 'component' config objects */
	static ConfigObject::Set::Ptr componentObjects = boost::make_shared<ConfigObject::Set>(ConfigObject::GetAllObjects(), ConfigObject::MakeTypePredicate("component"));
	componentObjects->OnObjectAdded.connect(boost::bind(&IcingaApplication::NewComponentHandler, this, _2));
	componentObjects->OnObjectCommitted.connect(boost::bind(&IcingaApplication::NewComponentHandler, this, _2));
	componentObjects->OnObjectRemoved.connect(boost::bind(&IcingaApplication::DeletedComponentHandler, this, _2));
	componentObjects->Start();

	/* load config file */
	ConfigObject::Ptr fileComponentConfig = boost::make_shared<ConfigObject>("component", "configfile");
	fileComponentConfig->SetLocal(true);
	fileComponentConfig->SetProperty("configFilename", args[1]);
	fileComponentConfig->Commit();

	ConfigObject::Ptr icingaConfig = ConfigObject::GetObject("application", "icinga");

	if (!icingaConfig)
		throw runtime_error("Configuration must contain an 'application' object named 'icinga'.");

	if (!icingaConfig->IsLocal())
		throw runtime_error("'icinga' application object must be 'local'.");

	icingaConfig->GetProperty("cert", &m_CertificateFile);
	icingaConfig->GetProperty("ca", &m_CAFile);
	icingaConfig->GetProperty("node", &m_Node);
	icingaConfig->GetProperty("service", &m_Service);

	if (!GetCertificateFile().empty() && !GetCAFile().empty()) {
		/* set up SSL context */
		shared_ptr<X509> cert = Utility::GetX509Certificate(GetCertificateFile());
		string identity = Utility::GetCertificateCN(cert);
		Application::Log(LogInformation, "icinga", "My identity: " + identity);
		m_EndpointManager->SetIdentity(identity);

		shared_ptr<SSL_CTX> sslContext = Utility::MakeSSLContext(GetCertificateFile(), GetCertificateFile(), GetCAFile());
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

void IcingaApplication::NewComponentHandler(const ConfigObject::Ptr& object)
{
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
}

void IcingaApplication::DeletedComponentHandler(const ConfigObject::Ptr& object)
{
	Component::Ptr component = GetComponent(object->GetName());
	UnregisterComponent(component);
}

string IcingaApplication::GetCertificateFile(void) const
{
	return m_CertificateFile;
}

string IcingaApplication::GetCAFile(void) const
{
	return m_CAFile;
}

string IcingaApplication::GetNode(void) const
{
	return m_Node;
}

string IcingaApplication::GetService(void) const
{
	return m_Service;
}
