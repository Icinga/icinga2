#include <cstdio>
#include <iostream>
#include "i2-icinga.h"

#ifndef _WIN32
#	include "icinga-version.h"
#	define ICINGA_VERSION GIT_MESSAGE
#endif /* _WIN32 */

using namespace icinga;

int IcingaApplication::Main(const vector<string>& args)
{
#ifdef _WIN32
	Application::Log("Icinga component loader");
#else /* _WIN32 */
	Application::Log("Icinga component loader (version: " ICINGA_VERSION ")");
#endif  /* _WIN32 */

	if (args.size() < 2) {
		PrintUsage(args[0]);
		return EXIT_FAILURE;
	}

	m_EndpointManager = make_shared<EndpointManager>();

	string componentDirectory = GetExeDirectory() + "/../lib/icinga";
	AddComponentSearchDir(componentDirectory);

	/* register handler for 'icinga' config objects */
	ConfigCollection::Ptr icingaCollection = GetConfigHive()->GetCollection("icinga");
	function<int (const EventArgs&)> NewIcingaConfigHandler = bind_weak(&IcingaApplication::NewIcingaConfigHandler, shared_from_this());
	icingaCollection->OnObjectCreated += NewIcingaConfigHandler;
	icingaCollection->ForEachObject(NewIcingaConfigHandler);
	icingaCollection->OnObjectRemoved += bind_weak(&IcingaApplication::DeletedIcingaConfigHandler, shared_from_this());

	/* register handler for 'component' config objects */
	ConfigCollection::Ptr componentCollection = GetConfigHive()->GetCollection("component");
	function<int (const EventArgs&)> NewComponentHandler = bind_weak(&IcingaApplication::NewComponentHandler, shared_from_this());
	componentCollection->OnObjectCreated += NewComponentHandler;
	componentCollection->ForEachObject(NewComponentHandler);
	componentCollection->OnObjectRemoved += bind_weak(&IcingaApplication::DeletedComponentHandler, shared_from_this());

	/* load config file */
	ConfigObject::Ptr fileComponentConfig = make_shared<ConfigObject>("component", "configfile");
	fileComponentConfig->SetPropertyString("configFilename", args[1]);
	fileComponentConfig->SetPropertyInteger("replicate", 0);
	GetConfigHive()->AddObject(fileComponentConfig);

	if (GetPrivateKeyFile().empty())
		throw InvalidArgumentException("No private key was specified.");

	if (GetPublicKeyFile().empty())
		throw InvalidArgumentException("No public certificate was specified.");

	if (GetCAKeyFile().empty())
		throw InvalidArgumentException("No CA certificate was specified.");

	/* set up SSL context */
	shared_ptr<X509> cert = Utility::GetX509Certificate(GetPublicKeyFile());
	string identity = Utility::GetCertificateCN(cert);
	Application::Log("My identity: " + identity);
	m_EndpointManager->SetIdentity(identity);

	shared_ptr<SSL_CTX> sslContext = Utility::MakeSSLContext(GetPublicKeyFile(), GetPrivateKeyFile(), GetCAKeyFile());
	m_EndpointManager->SetSSLContext(sslContext);

	/* create the primary RPC listener */
	string service = GetService();
	if (!service.empty())
		GetEndpointManager()->AddListener(service);

	RunEventLoop();

	return EXIT_SUCCESS;
}

void IcingaApplication::PrintUsage(const string& programPath)
{
	cout << "Syntax: " << programPath << " <config-file>" << endl;
}

EndpointManager::Ptr IcingaApplication::GetEndpointManager(void)
{
	return m_EndpointManager;
}

int IcingaApplication::NewComponentHandler(const EventArgs& ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);
	
	/* don't allow replicated config objects */
	if (object->GetReplicated())
		return 0;

	string path;
	if (!object->GetPropertyString("path", &path)) {
#ifdef _WIN32
		path = object->GetName() + ".dll";
#else /* _WIN32 */
		path = object->GetName() + ".la";
#endif /* _WIN32 */
	}

	LoadComponent(path, object);

	return 0;
}

int IcingaApplication::DeletedComponentHandler(const EventArgs& ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);

	Component::Ptr component = GetComponent(object->GetName());
	UnregisterComponent(component);

	return 0;
}

int IcingaApplication::NewIcingaConfigHandler(const EventArgs& ea)
{
	ConfigObject::Ptr object = static_pointer_cast<ConfigObject>(ea.Source);
	
	/* don't allow replicated config objects */
	if (object->GetReplicated())
		return 0;

	string privkey;
	if (object->GetPropertyString("privkey", &privkey))
		SetPrivateKeyFile(privkey);

	string pubkey;
	if (object->GetPropertyString("pubkey", &pubkey))
		SetPublicKeyFile(pubkey);

	string cakey;
	if (object->GetPropertyString("cakey", &cakey))
		SetCAKeyFile(cakey);

	string node;
	if (object->GetPropertyString("node", &node))
		SetNode(node);

	string service;
	if (object->GetPropertyString("service", &service))
		SetService(service);

	return 0;
}

int IcingaApplication::DeletedIcingaConfigHandler(const EventArgs& ea)
{
	throw Exception("Unsupported operation.");

	return 0;
}

void IcingaApplication::SetPrivateKeyFile(string privkey)
{
	m_PrivateKeyFile = privkey;
}

string IcingaApplication::GetPrivateKeyFile(void) const
{
	return m_PrivateKeyFile;
}

void IcingaApplication::SetPublicKeyFile(string pubkey)
{
	m_PublicKeyFile = pubkey;
}

string IcingaApplication::GetPublicKeyFile(void) const
{
	return m_PublicKeyFile;
}

void IcingaApplication::SetCAKeyFile(string cakey)
{
	m_CAKeyFile = cakey;
}

string IcingaApplication::GetCAKeyFile(void) const
{
	return m_CAKeyFile;
}

void IcingaApplication::SetNode(string node)
{
	m_Node = node;
}

string IcingaApplication::GetNode(void) const
{
	return m_Node;
}

void IcingaApplication::SetService(string service)
{
	m_Service = service;
}

string IcingaApplication::GetService(void) const
{
	return m_Service;
}
